// Kahlo/Frida helper for the current x86_64 libauro.so IDA target.
// IDA offsets stay authoritative; emitted values are per-track runtime probes.
exports.start = function(params, ctx) {
    function emit(kind, payload, level) {
        try {
            ctx.emit(kind, payload, level || 'info');
        } catch (e) {
        }
    }

    function u32(ptr, off) {
        try {
            return ptr.add(off).readU32();
        } catch (e) {
            return null;
        }
    }

    function readPtr(ptr, off) {
        try {
            return ptr.add(off).readPointer().toString();
        } catch (e) {
            return null;
        }
    }

    var mod;
    try {
        mod = Process.getModuleByName('libauro.so');
    } catch (e) {
        emit('auro.error', { msg: 'libauro.so not loaded', error: String(e) }, 'error');
        return;
    }

    var base = mod.base;
    emit('auro.base', { base: base.toString(), size: mod.size, path: mod.path });

    var seen = {};
    function throttle(name, ms) {
        var now = Date.now();
        var interval = ms || 500;
        if (!seen[name] || now - seen[name] > interval) {
            seen[name] = now;
            return true;
        }
        return false;
    }

    function a3dengFields(self) {
        return {
            self: self.toString(),
            configured: u32(self, 0x30),
            inRate: u32(self, 0x34),
            block: u32(self, 0x38),
            mode: u32(self, 0x40),
            inMask: u32(self, 0x50),
            outRate: u32(self, 0x54),
            sampleType: u32(self, 0x58),
            api: readPtr(self, 0x70),
            inst: readPtr(self, 0x78)
        };
    }

    function hook(name, off, callbacks) {
        var addr = base.add(off);
        try {
            Interceptor.attach(addr, callbacks(addr));
            emit('auro.hook', { name: name, addr: addr.toString() });
        } catch (e) {
            emit('auro.hook.error', { name: name, addr: addr.toString(), error: String(e) }, 'error');
        }
    }

    hook('A3DENG.update', 0x319E60, function(addr) {
        return {
            onEnter: function(args) {
                if (!throttle('update'))
                    return;
                emit('auro.update', {
                    addr: addr.toString(),
                    self: a3dengFields(args[0]),
                    settings: args[1].toString(),
                    s0: u32(args[1], 0),
                    s4: u32(args[1], 4),
                    s8: u32(args[1], 8),
                    s12: u32(args[1], 12),
                    s16: u32(args[1], 16),
                    s20: u32(args[1], 20),
                    s24: u32(args[1], 24),
                    s28: u32(args[1], 28)
                });
            }
        };
    });

    hook('A3DENG.push', 0x31AF50, function(addr) {
        return {
            onEnter: function(args) {
                this.self = args[0];
                this.bytes = args[2].toInt32();
                if (throttle('push')) {
                    emit('auro.push.enter', {
                        addr: addr.toString(),
                        self: a3dengFields(args[0]),
                        input: args[1].toString(),
                        bytes: this.bytes
                    });
                }
            },
            onLeave: function(ret) {
                if (throttle('push.ret')) {
                    emit('auro.push.leave', {
                        ret: ret.toInt32(),
                        bytes: this.bytes,
                        self: a3dengFields(this.self)
                    });
                }
            }
        };
    });

    hook('A3DENG.pop_', 0x31B7F0, function(addr) {
        return {
            onEnter: function(args) {
                this.self = args[0];
                this.lenp = args[2];
                this.len0 = u32(args[2], 0);
                if (throttle('pop')) {
                    emit('auro.pop.enter', {
                        addr: addr.toString(),
                        self: a3dengFields(args[0]),
                        out: args[1].toString(),
                        len: this.len0
                    });
                }
            },
            onLeave: function(ret) {
                if (throttle('pop.ret')) {
                    emit('auro.pop.leave', {
                        ret: ret.toInt32(),
                        len0: this.len0,
                        len1: this.lenp.isNull() ? null : u32(this.lenp, 0),
                        self: a3dengFields(this.self)
                    });
                }
            }
        };
    });

    hook('A3DENG.get_output_info', 0x31B4E0, function(addr) {
        return {
            onEnter: function(args) {
                this.self = args[0];
            },
            onLeave: function(ret) {
                if (!throttle('output_info'))
                    return;
                var raw = ret.toString();
                var rawNumber = parseInt(raw, 16);
                var sampleType = rawNumber & 0xff;
                var sampleRateBits = rawNumber & 0xffffff00;
                var outputBlockCount = Math.floor(rawNumber / 0x100000000);
                emit('auro.output_info', {
                    addr: addr.toString(),
                    raw: raw,
                    sampleType: sampleType,
                    sampleRateBits: sampleRateBits,
                    outputBlockCount: outputBlockCount,
                    self: a3dengFields(this.self)
                });
            }
        };
    });

    function hookJni(name, off, argc) {
        hook(name, off, function(addr) {
            return {
                onEnter: function(args) {
                    if (!throttle(name))
                        return;
                    var values = [];
                    for (var i = 0; i < argc; ++i)
                        values.push(args[i].toString());
                    emit('jni.enter', { name: name, addr: addr.toString(), args: values });
                },
                onLeave: function(ret) {
                    if (throttle(name + '.ret'))
                        emit('jni.leave', { name: name, ret: ret.toString() });
                }
            };
        });
    }

    hookJni('A3DENG.JNI.Initialize', 0x318C90, 4);
    hookJni('A3DENG.JNI.Update2', 0x318D60, 5);
    hookJni('A3DENG.JNI.Push', 0x318FE0, 5);
    hookJni('A3DENG.JNI.Pop', 0x319120, 5);

    setInterval(function() {
        try {
            ctx.heartbeat();
        } catch (e) {
        }
    }, 5000);
};
