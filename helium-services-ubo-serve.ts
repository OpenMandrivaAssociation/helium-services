// OpenMandriva runs this service on bun. Upstream starts it with
// `deno serve`. Importing main.ts does not set import.meta.main, so the
// assets preload has to be called from here.
import app from "./main.ts";
import * as Assets from "./lib/ublock.ts";

const listen = (() => {
    const g = globalThis as {
        process?: { env: Record<string, string | undefined> };
    };
    return g.process?.env.UBO_LISTEN || "127.0.0.1:8092";
})();

const colon = listen.lastIndexOf(":");
if (colon <= 0) {
    throw new Error(`UBO_LISTEN: invalid value ${listen}`);
}
const hostname = listen.slice(0, colon);
const port = Number(listen.slice(colon + 1));
if (!Number.isInteger(port) || port <= 0 || port > 65535) {
    throw new Error(`UBO_LISTEN: invalid value ${listen}`);
}

Assets.handleAssets().catch((err: unknown) => {
    console.warn("ubo: assets preload failed", err);
});

Bun.serve({
    hostname,
    port,
    idleTimeout: 120,
    fetch(request: Request) {
        return app.fetch(request);
    },
});

console.log(`ubo listening on ${hostname}:${port}`);
