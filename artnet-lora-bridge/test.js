var dmxlib = require('dmxnet');
const SerialPort = require('serialport')
const Readline = SerialPort.parsers.Readline

const PORT = `/dev/cu.usbmodem143101`

var dmxnet = new dmxlib.dmxnet({
    ip: "127.0.0.1", // IP to send to, default 255.255.255.255
    // subnet: 0, // Destination subnet, default 0
    // universe: 0, // Destination universe, default 0
    // net: 0, // Destination net, default 0
    port: 6454, // Destination UDP Port, default 6454
    // base_refresh_interval: 1000 // Default interval for sending unchanged ArtDmx
});

var receiver = dmxnet.newReceiver({
    subnet: 0, // Destination subnet, default 0
    universe: 0, // Destination universe, default 0
    net: 0, // Destination net, default 0
});

const port = new SerialPort(PORT)
const parser = new Readline()

function send(command) {
    console.log('send command', command)
    port.write(`${command}\n`)
}

var lastLightState = {}

const hex = d => Number(d).toString(16).padStart(2, '0')

function handleLight(index, r,g,b) {
    var lightstate = lastLightState[index] || {}

    // console.log('handleLight', index, r,g,b)

    if (r != lightstate.r ||
        g != lightstate.g ||
        b != lightstate.b) {
        lightstate.r = r
        lightstate.g = g
        lightstate.b = b

        console.log('send color change', index, r,g,b)
        send(`43 5F ${hex(index)} 00 5F ${hex(r)} ${hex(g)} ${hex(b)}`)
    }

    lastLightState[index] = lightstate
}

function sendSetup(blackout) {
    // send('45 5F 00 03 5F 03 30 00')
    send('45 5F 00 03 5F 00 00 00')

    if (blackout) {
        send('43 5F 00 03 5F 00 00 00')
    }
}







var dmxcounter = 0;

receiver.on('data', function(data) {
    if (dmxcounter % 1 == 0) {
        console.log('DMX data:', data)
        for(var i=0; i<30; i++) {
            handleLight(
                i + 2,
                data[i * 3 + 0],
                data[i * 3 + 1],
                data[i * 3 + 2]
            )
        }
    }

    dmxcounter ++;
})

setInterval(() => sendSetup(false), 5000)
sendSetup(true)

port.pipe(parser)

parser.on('data', data => {
    console.log('Serial data: ', data)
})

port.write('00 00 00 00\n')

