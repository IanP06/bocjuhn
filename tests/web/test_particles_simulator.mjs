import fs from 'node:fs';
import assert from 'node:assert/strict';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
globalThis.require=createRequire(import.meta.url);
globalThis.__dirname=fileURLToPath(new URL('../../browser/particles/',import.meta.url));
const {default:create}=await import('../../browser/particles/firmware-core.mjs');
const core=await create({wasmBinary:fs.readFileSync(new URL('../../browser/particles/firmware-core.wasm',import.meta.url))});
core.HEAPU8.fill(255,core._sim_flash(),core._sim_flash()+0x100000);core._sim_boot();
const state=()=>Array.from(core.HEAP32.subarray(core._sim_state()/4,core._sim_state()/4+48));
const button=n=>{core._sim_button(n,1);core._sim_button(n,0);};
const key=n=>{core._sim_key(n,1);core._sim_key(n,0);};
function render(n=1){let energy=0;for(let b=0;b<n;b++){const at=core._sim_render(1024)/4;for(const x of core.HEAPF32.subarray(at,at+2048)){assert.ok(Number.isFinite(x));energy+=x*x;}}return energy;}
for(let e=0;e<8;e++){
  assert.equal(state()[1],e);core._sim_key(0,1);const energy=render(40);core._sim_key(0,0);render(5);
  assert.ok(energy>0.00001,'engine '+e+' produces audio');console.log('engine',e,'audio energy',energy.toFixed(3));
  if(e<7)core._sim_encoder(2,1);
}
button(11);core._sim_key(0,1);render(10);core._sim_key(0,0);render(10);button(11);render();
assert.equal(state()[8],1,'free loop recorded');
key(15);assert.equal(state()[4],1,'speed page');
core._sim_encoder(3,37);render();assert.equal(state()[9],137);
core._sim_encoder(5,-27);render();assert.equal(state()[10],73);
button(0);core._sim_key(25,1);core._sim_encoder(4,1);core._sim_key(25,0);
assert.equal(state()[3],1,'grain page');
for(let i=0;i<4;i++)core._sim_encoder(3+i,10+i);
assert.deepEqual(state().slice(16,20),[10,11,12,13]);
button(0);key(22);assert.equal(state()[2],1,'EDIT restores original macros');
core._sim_button(7,1);render(45);core._sim_button(7,0);button(7);render(5);
assert.ok(core._sim_flash_version()>0,'SAVE writes persistent flash');
assert.ok(core.HEAPU16.subarray(core._sim_screen()/2,core._sim_screen()/2+57600).some(x=>x),'real framebuffer rendered');
core._sim_panic();render(30);
console.log('simulator: eight engines, loops, speed, grains, macros, flash and framebuffer OK');
