'use strict';

const moduleName = 'libauro.so';
const runtimePresetSize = 0x428;
const engine1SourceTableSize = 1568;
const engine2BusPresetSize = 136;

const stereoRuntimePresets = [
  [0x656ED8, 0x656AB0, 0x657300, 0x657728, 0x657B50],
  [0x65A908, 0x65AD30, 0x65B158, 0x65B580, 0x65B9A8],
  [0x659440, 0x659868, 0x659C90, 0x65A0B8, 0x65A4E0],
  [0x6583A0, 0x6587C8, 0x657F78, 0x658BF0, 0x659018],
  [0x65C620, 0x65C1F8, 0x65BDD0, 0x65CA48, 0x65CE70],
];

const surroundRuntimePresets = [
  [0x6502C8, 0x650B18, 0x6506F0, 0x650F40, 0x651368],
  [0x654120, 0x654548, 0x654970, 0x654D98, 0x6551C0],
  [0x653080, 0x652C58, 0x6534A8, 0x6538D0, 0x653CF8],
  [0x651BB8, 0x651790, 0x651FE0, 0x652408, 0x652830],
  [0x656260, 0x6555E8, 0x655A10, 0x655E38, 0x656688],
];

function bytesToBase64(ptrValue, size) {
  const bytes = Memory.readByteArray(ptrValue, size);
  return bytes ? bytes.toString('base64') : '';
}

function readU64(ptrValue, offset) {
  return Memory.readPointer(ptrValue.add(offset));
}

function emit(obj) {
  console.log('AURO_XINN_PRESET_DUMP ' + JSON.stringify(obj));
}

function dumpPreset(kind, family, roomSlot, imageOffset) {
  const base = Module.findBaseAddress(moduleName);
  if (base === null) {
    emit({ type: 'error', message: moduleName + ' not loaded' });
    return;
  }
  const preset = base.add(imageOffset);
  const record = {
    type: 'preset',
    kind,
    family,
    roomSlot,
    imageOffset: '0x' + imageOffset.toString(16),
    moduleBase: base.toString(),
    payload: bytesToBase64(preset, runtimePresetSize),
    engine1Tables: [],
    engine2Buses: [],
  };

  [40, 48, 56].forEach((off, idx) => {
    const p = readU64(preset, off);
    record.engine1Tables.push({
      index: idx,
      pointer: p.toString(),
      payload: p.isNull() ? '' : bytesToBase64(p, engine1SourceTableSize),
    });
  });

  [1040, 1048, 1056].forEach((off, idx) => {
    const p = readU64(preset, off);
    record.engine2Buses.push({
      index: idx,
      pointer: p.toString(),
      payload: p.isNull() ? '' : bytesToBase64(p, engine2BusPresetSize),
    });
  });

  emit(record);
}

setImmediate(() => {
  const base = Module.findBaseAddress(moduleName);
  emit({
    type: 'module',
    name: moduleName,
    base: base ? base.toString() : null,
  });
  if (base === null)
    return;

  stereoRuntimePresets.forEach((rooms, family) => {
    rooms.forEach((offset, roomSlot) => dumpPreset('stereo', family, roomSlot, offset));
  });
  surroundRuntimePresets.forEach((rooms, family) => {
    rooms.forEach((offset, roomSlot) => dumpPreset('surround', family, roomSlot, offset));
  });
  emit({ type: 'done' });
});
