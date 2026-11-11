import createTonitor from './firmware-core.mjs?v=rc9-dev';
let core, lastFrame = 0, flashVersion = 0;
function frame(force = false) {
  if (!core || (!force && performance.now() - lastFrame < 33)) return;
  lastFrame = performance.now();
  const pixels = core.HEAPU16.slice(core._sim_screen() / 2, core._sim_screen() / 2 + 57600);
  const leds = core.HEAPU8.slice(core._sim_leds(), core._sim_leds() + 64);
  const state = core.HEAP32.slice(core._sim_state() / 4, core._sim_state() / 4 + 56);
  postMessage({type: 'frame', pixels, leds, state}, [pixels.buffer, leds.buffer, state.buffer]);
  const version = core._sim_flash_version();
  if (version !== flashVersion) {
    flashVersion = version;
    const flash = core.HEAPU8.slice(core._sim_flash(), core._sim_flash() + 0x100000);
    postMessage({type: 'flash', flash}, [flash.buffer]);
  }
}
self.onmessage = async ({data: d}) => {
  try {
    if (d.type === 'boot') {
      core = await createTonitor({locateFile: path => new URL(path+'?v=rc9-dev',import.meta.url).href,
        printErr: text => postMessage({type:'log', text})});
      const at = core._sim_flash();
      core.HEAPU8.fill(255, at, at + 0x100000);
      if (d.flash?.byteLength === 0x100000) core.HEAPU8.set(new Uint8Array(d.flash), at);
      core._sim_boot(); flashVersion = core._sim_flash_version();
      postMessage({type:'ready'}); frame(true); return;
    }
    if (!core) return;
    if (d.type === 'render') {
      const frames = 1024, at = core._sim_render(frames) / 4;
      const samples = core.HEAPF32.slice(at, at + frames * 2);
      postMessage({type:'audio', samples}, [samples.buffer]); frame(); return;
    }
    const handlers = {key: '_sim_key', button: '_sim_button', encoder: '_sim_encoder',
      volume: '_sim_volume', release: '_sim_release', panic: '_sim_panic'};
    if (handlers[d.type]) core[handlers[d.type]](...(d.args || []));
    frame(true);
  } catch (error) { postMessage({type:'error', text: String(error.stack || error)}); }
};
