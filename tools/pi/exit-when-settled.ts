/**
 * Quit pi once the agent finishes its work, so tools/scripts/pi_loop.sh can start the next pass.
 *
 * Does nothing unless PI_LOOP_MARKER is set. When a run settles on its own, the marker file is
 * created and pi shuts down. A run aborted with Escape leaves pi open; quitting by hand after
 * that creates no marker, which is how the loop knows to stop.
 */

import { writeFileSync } from "node:fs";
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";

export default function (pi: ExtensionAPI) {
	const marker = process.env.PI_LOOP_MARKER;
	if (!marker) return;

	pi.on("agent_settled", async (event, ctx) => {
		if (event.aborted) return;
		writeFileSync(marker, "");
		ctx.shutdown();
	});
}
