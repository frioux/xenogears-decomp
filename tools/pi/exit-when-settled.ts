/**
 * Quit pi once the agent finishes its work, so tools/scripts/pi_loop.sh can start the next pass.
 *
 * Does nothing unless PI_LOOP_MARKER is set. When a run settles on its own, the marker file is
 * created and pi shuts down. A run aborted with Escape leaves pi open; quitting by hand after
 * that creates no marker, which is how the loop knows to stop.
 *
 * "Settled" is not "finished" while a pi-subagents async run is out: the orchestrator ends its
 * turn to wait for the completion notice, and the workflow script runs inside this pi process.
 * Quitting then kills the round and orphans its detached workers, so pi stays open until every
 * async run this session launched has completed.
 */

import { writeFileSync } from "node:fs";
import type { ExtensionAPI, ExtensionContext } from "@earendil-works/pi-coding-agent";

// How long to wait for a completion notice to start a turn before quitting an idle pi.
const IDLE_GRACE_MS = 30_000;

export default function (pi: ExtensionAPI) {
	const marker = process.env.PI_LOOP_MARKER;
	if (!marker) return;

	const outstanding = new Set<string>();
	let idleCtx: ExtensionContext | undefined;
	let idleTimer: ReturnType<typeof setTimeout> | undefined;

	const finish = (ctx: ExtensionContext) => {
		writeFileSync(marker, "");
		ctx.shutdown();
	};

	// A workflow's children are awaited by the workflow itself; only top-level runs are tracked.
	pi.events.on("subagent:async-started", (data: any) => {
		if (data?.id && !data.parentWorkflowRunId) outstanding.add(data.id);
	});

	pi.events.on("subagent:async-complete", (data: any) => {
		outstanding.delete(data?.id);
		outstanding.delete(data?.runId);
		if (outstanding.size || !idleCtx) return;
		// The notice normally starts a turn, which settles and quits below. If it does not,
		// nothing else will ever wake this session.
		const ctx = idleCtx;
		clearTimeout(idleTimer);
		idleTimer = setTimeout(() => {
			if (idleCtx === ctx && !outstanding.size && ctx.isIdle()) finish(ctx);
		}, IDLE_GRACE_MS);
	});

	pi.on("agent_start", async () => {
		idleCtx = undefined;
		clearTimeout(idleTimer);
	});

	pi.on("agent_settled", async (event, ctx) => {
		if (event.aborted) return;
		if (outstanding.size) {
			idleCtx = ctx;
			ctx.ui.notify(`pi_loop: waiting on ${outstanding.size} async subagent run(s) before the next pass`, "info");
			return;
		}
		finish(ctx);
	});
}
