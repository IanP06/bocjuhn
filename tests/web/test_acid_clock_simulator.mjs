import fs from 'node:fs';
import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
globalThis.require=createRequire(import.meta.url);
globalThis.__dirname=fileURLToPath(new URL('../../browser/',import.meta.url));
const {default:create}=await import('../../browser/firmware-core.mjs');
const c=await create({wasmBinary:fs.readFileSync(new URL('../../browser/firmware-core.wasm',import.meta.url))});
c.HEAPU8.fill(255,c._sim_flash(),c._sim_flash()+0x100000);c._sim_boot();
const state=()=>Array.from(c.HEAP32.subarray(c._sim_state()/4,c._sim_state()/4+56));
const button=n=>{c._sim_button(n,1);c._sim_button(n,0);};
const render=n=>{let energy=0;while(n--){const at=c._sim_render(1024)/4;for(const x of c.HEAPF32.subarray(at,at+2048)){assert(Number.isFinite(x));energy+=x*x;}}return energy;};
button(1);assert.equal(state()[1],1);
c._sim_key(0,1);assert(render(20)>0.001);c._sim_key(0,0);render(20);
for(let i=0;i<7;i++) {
  button(9);
  if(i<4) {assert.equal(state()[20],i%2+1);assert.equal(state()[21],i<2?1:0);}
}
assert.equal(state()[48],1,'SEQ seventh page CLOCK');assert.equal(state()[50],120);
c._sim_encoder(3,13);assert.equal(state()[50],133);
c._sim_encoder(4,2);assert.equal(state()[51],2);
c._sim_encoder(5,1);assert.equal(state()[52],1);
c._sim_encoder(6,1);assert.equal(state()[53],4);
button(2);assert.equal(state()[48],1);assert.equal(state()[52],0,'other track remains FREE');
button(1);assert.equal(state()[52],1,'track clock retained');
button(9);assert.equal(state()[49],1,'ACID settings follows CLOCK');
button(9);assert.equal(state()[49],0,'SEQ exits');
c._sim_key(25,1);c._sim_encoder(4,1);c._sim_key(25,0);
assert.equal(state()[3],1,'GRAINS page');button(9);assert.equal(state()[3],2,'PRISM page');
c._sim_encoder(6,20);assert.equal(state()[19],20,'PRISM mix reflected in simulator controls');
c._sim_panic();render(100);
console.log('WASM ACID/CLOCK: audio, page traversal, BPM, per-target config and track isolation PASS');
