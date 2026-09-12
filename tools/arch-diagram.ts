/*
 * Renders the module graph of this C project as an SVG.
 *
 *   node tools/arch-diagram.ts [out.svg]    (or: make diagram)
 *
 * Nothing about the graph is hardcoded: the sources are scanned for local
 * `#include "..."` directives, sibling .c/.h files are folded into one module,
 * and a module's height is its longest dependency chain, so the picture stays
 * honest when the code moves. Horizontal lanes are the source directories.
 */

import { readFileSync, readdirSync, writeFileSync, statSync } from "node:fs";
import { join, relative, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const ROOT = join(dirname(fileURLToPath(import.meta.url)), "..");

/* --- model ---------------------------------------------------------------- */

type Layer = "util" | "ds" | "core" | "app" | "root";

interface Module {
	id: string; /* "core/task", "print" — repo path without extension */
	layer: Layer;
	files: string[];
	loc: number;
	api: number; /* functions declared in the header */
	deps: Map<string, Dep>;
	hasHeader: boolean;
	depth: number; /* longest chain of dependencies underneath */
	x: number;
	y: number;
}

interface Dep {
	viaHeader: boolean; /* the header needs it, not just the .c */
	cyclic: boolean; /* closes an include cycle */
}

/* Lanes, left to right: foundations first, entry point last. */
const LAYERS: Record<Layer, { label: string; hue: number }> = {
	util: { label: "util", hue: 36 },
	ds: { label: "ds", hue: 158 },
	core: { label: "core", hue: 212 },
	app: { label: "app", hue: 292 },
	root: { label: "entry / io", hue: 350 },
};
const LAYER_ORDER = Object.keys(LAYERS) as Layer[];

const light = (hue: number) => `hsl(${hue} 64% 42%)`;
const dark = (hue: number) => `hsl(${hue} 70% 64%)`;

/* --- scan ----------------------------------------------------------------- */

function sources(dir: string, found: string[] = []): string[] {
	for (const entry of readdirSync(dir)) {
		if (entry === "node_modules" || entry.startsWith(".")) continue;
		const path = join(dir, entry);
		if (statSync(path).isDirectory()) sources(path, found);
		else if (/\.[ch]$/.test(entry)) found.push(path);
	}
	return found;
}

const moduleId = (path: string) =>
	relative(ROOT, path).replace(/^src\//, "").replace(/\.[ch]$/, "");

function layerOf(id: string): Layer {
	const dir = id.includes("/") ? (id.split("/")[0] as Layer) : "root";
	return dir in LAYERS ? dir : "root";
}

/* Public API surface: declarations left after comments and type definitions. */
function countDeclarations(header: string): number {
	return header
		.replace(/\/\*[\s\S]*?\*\//g, "")
		.replace(/^\s*#.*$/gm, "")
		.replace(/typedef[\s\S]*?;/g, "")
		.split(";")
		.filter((decl) => /\w[\w\s*]*\b\w+\s*\([\s\S]*\)/.test(decl)).length;
}

/* Modules without a header (main.c) publish their non-static definitions. */
function countDefinitions(source: string): number {
	const bodies = source
		.replace(/\/\*[\s\S]*?\*\//g, "")
		.matchAll(/^([A-Za-z_][\w \t*]*?)\b\w+\s*\([^;{]*\)\s*\{/gm);
	return [...bodies].filter(([, prefix]) => !/\bstatic\b/.test(prefix)).length;
}

function scan(): Map<string, Module> {
	const modules = new Map<string, Module>();

	for (const path of sources(ROOT).sort()) {
		const id = moduleId(path);
		const text = readFileSync(path, "utf8");
		const mod: Module =
			modules.get(id) ??
			{ id, layer: layerOf(id), files: [], loc: 0, api: 0,
			  deps: new Map(), hasHeader: false, depth: 0, x: 0, y: 0 };

		mod.files.push(relative(ROOT, path));
		mod.loc += text.split("\n").length;
		const isHeader = path.endsWith(".h");
		if (isHeader) {
			mod.hasHeader = true;
			mod.api = countDeclarations(text);
		} else if (!mod.hasHeader) {
			mod.api = countDefinitions(text);
		}

		for (const [, included] of text.matchAll(/^\s*#include\s+"([^"]+)"/gm)) {
			const dep = included.replace(/\.h$/, "");
			if (dep === id) continue;
			const prev = mod.deps.get(dep);
			mod.deps.set(dep, {
				viaHeader: (prev?.viaHeader ?? false) || isHeader,
				cyclic: false,
			});
		}
		modules.set(id, mod);
	}

	/* Drop includes of anything that is not a module we actually found. */
	for (const mod of modules.values())
		for (const dep of [...mod.deps.keys()])
			if (!modules.has(dep)) mod.deps.delete(dep);

	return modules;
}

/* --- layout --------------------------------------------------------------- */

const BOX_W = 178;
const BOX_H = 60;
const GAP_X = 24;
const GAP_Y = 50;
const LANE_PAD = 14;
const PAD = 34;
const HEADER_H = 122;
const FOOTER_H = 44;

/* depth = longest dependency chain below a module; back edges are the cycles. */
function assignDepths(modules: Map<string, Module>): number {
	const done = new Map<string, number>();
	const onPath = new Set<string>();

	const depthOf = (id: string): number => {
		const memo = done.get(id);
		if (memo !== undefined) return memo;
		const mod = modules.get(id)!;
		if (onPath.has(id)) return 0; /* include cycle: break it here */
		onPath.add(id);
		let depth = 0;
		for (const [dep, edge] of mod.deps) {
			if (onPath.has(dep)) {
				edge.cyclic = true;
				continue;
			}
			depth = Math.max(depth, depthOf(dep) + 1);
		}
		onPath.delete(id);
		done.set(id, depth);
		mod.depth = depth;
		return depth;
	};

	let max = 0;
	for (const id of modules.keys()) max = Math.max(max, depthOf(id));
	return max;
}

interface Lane {
	layer: Layer;
	slots: number; /* modules that share a depth need side-by-side slots */
	x: number;
	w: number;
}

function layout(modules: Map<string, Module>) {
	const maxDepth = assignDepths(modules);
	const all = [...modules.values()];
	const rowY = (depth: number) =>
		HEADER_H + (maxDepth - depth) * (BOX_H + GAP_Y);

	const lanes: Lane[] = LAYER_ORDER.filter((layer) =>
		all.some((m) => m.layer === layer)
	).map((layer) => {
		const members = all.filter((m) => m.layer === layer);
		const perDepth = new Map<number, number>();
		for (const m of members)
			perDepth.set(m.depth, (perDepth.get(m.depth) ?? 0) + 1);
		return { layer, slots: Math.max(...perDepth.values()), x: 0, w: 0 };
	});

	let x = PAD;
	for (const lane of lanes) {
		lane.w = lane.slots * BOX_W + (lane.slots - 1) * GAP_X + LANE_PAD * 2;
		lane.x = x;
		x += lane.w + GAP_X;
	}
	const canvasW = x - GAP_X + PAD;
	const canvasH = rowY(0) + BOX_H + FOOTER_H;

	for (const lane of lanes) {
		const members = all
			.filter((m) => m.layer === lane.layer)
			.sort((a, b) => a.depth - b.depth || a.id.localeCompare(b.id));
		const used = new Map<number, number>();
		for (const mod of members) {
			const slot = used.get(mod.depth) ?? 0;
			used.set(mod.depth, slot + 1);
			const rowSlots = members.filter((m) => m.depth === mod.depth).length;
			const rowW = rowSlots * BOX_W + (rowSlots - 1) * GAP_X;
			mod.x = lane.x + (lane.w - rowW) / 2 + slot * (BOX_W + GAP_X);
			mod.y = rowY(mod.depth);
		}
	}

	return { lanes, maxDepth, canvasW, canvasH, rowY };
}

/* --- render --------------------------------------------------------------- */

const esc = (s: string) =>
	s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

/* Leave each box from its bottom edge and arrive at the dependency's top edge,
 * nudged sideways so parallel edges into a popular module stay distinct. */
function edgePath(from: Module, to: Module): string {
	const spread = Math.max(-52, Math.min(52, (from.x - to.x) / 6));
    const x1 = from.x + BOX_W / 2 + spread / 3;
	const y1 = from.y + BOX_H;
	const x2 = to.x + BOX_W / 2 + spread;
	const y2 = to.y;
	const lift = Math.max(28, (y2 - y1) / 2);
	return `M ${x1.toFixed(1)} ${y1} C ${x1.toFixed(1)} ${(y1 + lift).toFixed(1)}, ` +
		`${x2.toFixed(1)} ${(y2 - lift).toFixed(1)}, ${x2.toFixed(1)} ${y2}`;
}

function styles(): string {
	const rule = (fn: (hue: number) => string) =>
		Object.entries(LAYERS)
			.flatMap(([name, { hue }]) => [
				`.f-${name} { fill: ${fn(hue)}; }`,
				`.s-${name} { stroke: ${fn(hue)}; }`,
			])
			.join("\n\t");

	return `<style>
	.bg { fill: #fbfbfd; }
	.lane { fill: #f1f2f6; }
	.lane-label { font-size: 11px; font-weight: 650; letter-spacing: .08em; }
	.title { font-size: 20px; font-weight: 650; fill: #16181d; }
	.sub { font-size: 12px; fill: #646b78; }
	.name { font-size: 13.5px; font-weight: 600; fill: #16181d; }
	.meta { font-size: 10.5px; fill: #646b78; }
	.box { fill: #ffffff; stroke: #d7dae1; stroke-width: 1; }
	.edge { fill: none; stroke-width: 1.3; opacity: .42; }
	.edge.api { stroke-width: 1.7; opacity: .62; }
	.edge.impl { stroke-dasharray: 3 4; }
	.edge.cyclic { stroke: #e0303c; stroke-width: 2; opacity: .9; stroke-dasharray: 6 3; }
	.tick { font-size: 10px; fill: #9aa1ad; }
	${rule(light)}
	@media (prefers-color-scheme: dark) {
		.bg { fill: #14161a; }
		.lane { fill: #1a1d23; }
		.title, .name { fill: #eef0f4; }
		.sub, .meta { fill: #99a1ae; }
		.box { fill: #1f2228; stroke: #333942; }
		.tick { fill: #6d7683; }
		${rule(dark).replace(/\n\t/g, "\n\t\t")}
	}
</style>`;
}

function render(modules: Map<string, Module>): string {
	const { lanes, maxDepth, canvasW, canvasH, rowY } = layout(modules);
	const all = [...modules.values()];
	const edges = all.flatMap((m) =>
		[...m.deps].map(([id, edge]) => ({ from: m, to: modules.get(id)!, ...edge })));
	const cycles = edges.filter((e) => e.cyclic);
	const loc = all.reduce((n, m) => n + m.loc, 0);
	const api = all.reduce((n, m) => n + m.api, 0);
	const files = all.reduce((n, m) => n + m.files.length, 0);

	const out: string[] = [];
	out.push(
		`<svg xmlns="http://www.w3.org/2000/svg" width="${canvasW}" height="${canvasH}" ` +
			`viewBox="0 0 ${canvasW} ${canvasH}" ` +
			`font-family="ui-sans-serif, -apple-system, 'Segoe UI', Roboto, Helvetica, sans-serif">`,
		styles(),
		`<rect class="bg" width="${canvasW}" height="${canvasH}"/>`,
		`<text class="title" x="${PAD}" y="40">hello — module graph</text>`,
		`<text class="sub" x="${PAD}" y="61">${files} files · ${loc.toLocaleString()} lines · ` +
			`${api} public functions · ${edges.length} include edges</text>`,
		`<text class="sub" x="${PAD}" y="79">lanes are source directories · ` +
			`a module sits above everything it includes · ` +
			`solid = header dependency, dashed = implementation only` +
			`${cycles.length ? " · red = include cycle" : ""}</text>`,
	);

	/* lanes */
	for (const lane of lanes) {
		const { label, hue } = LAYERS[lane.layer];
		out.push(
			`<rect class="lane" x="${lane.x}" y="${HEADER_H - 34}" width="${lane.w}" ` +
				`height="${canvasH - HEADER_H - FOOTER_H + 62}" rx="12"/>`,
			`<text class="lane-label f-${lane.layer}" x="${lane.x + lane.w / 2}" ` +
				`y="${HEADER_H - 16}" text-anchor="middle">${esc(label.toUpperCase())}</text>`,
		);
		void hue;
	}

	/* depth ticks down the right margin */
	for (let depth = 0; depth <= maxDepth; depth++)
		out.push(
			`<text class="tick" x="${canvasW - PAD + 22}" y="${rowY(depth) + BOX_H / 2 + 4}" ` +
				`text-anchor="end">${depth === 0 ? "leaf" : depth}</text>`,
		);

	for (const edge of edges)
		out.push(
			`<path class="edge ${edge.cyclic ? "cyclic" : edge.viaHeader ? "api" : "impl"} ` +
				`s-${edge.from.layer}" d="${edgePath(edge.from, edge.to)}"/>`,
		);

	for (const mod of all) {
		const base = mod.id.includes("/") ? mod.id.split("/")[1] : mod.id;
		const deps = mod.deps.size;
		const users = all.filter((m) => m.deps.has(mod.id)).length;
		out.push(
			`<g><title>${esc(mod.files.join(", "))}</title>`,
			`<rect class="box" x="${mod.x}" y="${mod.y}" width="${BOX_W}" height="${BOX_H}" rx="9"/>`,
			`<rect class="f-${mod.layer}" x="${mod.x}" y="${mod.y}" width="4" ` +
				`height="${BOX_H}" rx="2"/>`,
			`<text class="name" x="${mod.x + 16}" y="${mod.y + 25}">${esc(base)}</text>`,
			`<text class="meta" x="${mod.x + 16}" y="${mod.y + 43}">` +
				`${mod.api} fn · ${mod.loc} loc · ${deps} in · ${users} out</text>`,
			`</g>`,
		);
	}

	out.push(
		`<text class="sub" x="${PAD}" y="${canvasH - 16}">` +
			`"in" = modules this one includes, "out" = modules that include it` +
			`${cycles.length
				? ` · ${cycles.length} cycle edge${cycles.length === 1 ? "" : "s"}: ` +
				  cycles.map((c) => `${c.from.id} ⇄ ${c.to.id}`).join(", ")
				: ""}</text>`,
		`</svg>`,
	);
	return out.join("\n") + "\n";
}

/* --- main ----------------------------------------------------------------- */

const out = process.argv[2] ?? join(ROOT, "docs", "architecture.svg");
const modules = scan();
writeFileSync(out, render(modules));
const edgeCount = [...modules.values()].reduce((n, m) => n + m.deps.size, 0);
console.log(`wrote ${relative(ROOT, out)} — ${modules.size} modules, ${edgeCount} edges`);
