/*
 * The MIT License
 *
 * Copyright 2018 Kirill Scherba <kirill@scherba.ru>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

'use strict';

var teonet = require('teonet/teonet');
var logger = teonet.syslog('teoloadsi-js', module.filename);

var ref = require('ref');
var ArrayType = require('ref-array');
var StructType = require('ref-struct');

/**
 * This application API commands
 */
var teoApi = {
    CMD_ECHO_ANSWER: 66,
    CMD_T: 129
};

var _ke; // right pointer to ksnetEvMgrClass
var peers = Object.create(null);

const SERVER_PEER = "teo-load-si";
const BUFFER_SIZE = 128;
const NUM_TO_SHOW = 1000;
const NUM_RUNDOM = 5;
var SEND_IN_MAIN = 1;
var start_f = 1;
var send_f = 0;
var errors = 0;
var id = 0;

// Application welcome message
console.log("Teoloadsi-js ver. 0.0.1, based on teonet ver. " + teonet.version());

// Start teonet module
teo_main();

// ----------------------------------------------------------------------------


var cmdTData = StructType({

    id: 'uint32', ///! Packet id
    type: 'uint8', ///! Packet type: 0 - request, 1 - response
    data: ArrayType('char', BUFFER_SIZE) ///! Data buffer

});
//var cmd_dataPtr = ref.refType(cmd_data);


function getRandomInt(min, max) {
  min = Math.ceil(min);
  max = Math.floor(max);
  return Math.floor(Math.random() * (max - min)) + min; //The maximum is exclusive and the minimum is inclusive
}

function sendRandom(packets) {
    if(packets) {
        const r = getRandomInt(1, packets);
        for(var i = 0; i < r; i++)
            teonet.sendCmdToBinaryA(_ke, SERVER_PEER, teoApi.CMD_T+1, 
                Buffer.from("hello"), 5);
    }
}

/**
 * Send command with simple load test data
 *
 * @param ke Pointer to ksnetEvMgrClass
 * @param to Peer name
 * @param id Packet id
 * @param type Packet type: 0 - request, 1 - response
 */
function sendCmdT(ke, to, id, type) {

    // Prepare data
    const d = cmdTData();
    d.id = id;
    d.type = type;
    //strncpy(d.data, "Any commands data", BUFFER_SIZE);
    //d.data = CharArray.untilZeros(Buffer.from("Hello!")); // Uint8Array.from(Buffer.from('Hello!'));

    // Show message
    if(id === 1 || !(id%NUM_TO_SHOW))
        console.log(` sendCmdT:      ->> packet #${id} ->> '${to}'`);

    // Send command
    teonet.sendCmdToBinary(ke, to, teoApi.CMD_T, d['ref.buffer'], d['ref.buffer'].length);
    send_f = 0;
}

function test_sendCmdAnswerToBinary(ke) {
    // rd, cmd, data, data_length
    const rd = { l0_f:0, from:"me\0", from_len: 3, addr:"127.0.0.1\0", port:7755 };
    const cmd = 129;
    const data = Buffer.from("hello\0");
    const data_length = 6;
    teonet.sendCmdAnswerToBinary(ke, rd, cmd, data, data_length);
}

function test_subscribe(ke) {
    // f_type, cmd, peer_length, ev, peer
    const rd = { l0_f:0, from:"me\0", from_len: 3, addr:"127.0.0.1\0", port:7755 };
    const cmd = 129;
    const data = Buffer.from("hello\0");
    const data_length = 6;
    teonet.subscribe(ke, "my-peer", 32768);
}

function test(_ke) {
    console.log("\nstatrt\n");

    teonet.sendCmdTo(_ke, "teo-load-si-6", 130, `{"packetId":12,"data":{"userId":"5b30efeed8c69f05913fe0b7","key":"playerDisciples"}}`);

    teonet.subscribe(_ke, "teo-load-si-6", 0x8000);

    teonet.sendToSscr(_ke, 0x8000, "hello", 5, 0);            
}

/**
 * Teonet event callback
 *
 * Original C function parameters:
 * void roomEventCb(ksnetEvMgrClass *ke, ksnetEvMgrEvents event, void *data, size_t data_len, void *user_data)
 *
 * @param {pointer} ke Pointer to ksnetEvMgrClass, see the http://repo.ksproject.org/docs/teonet/structksnetEvMgrClass.html
 * @param {int} ev Teonet event number, see the http://repo.ksproject.org/docs/teonet/ev__mgr_8h.html#ad7b9bff24cb809ad64c305b3ec3a21fe
 * @param {pointer} data Binary or string (depended on event) data
 * @param {int} data_len Data length
 * @param {pointer} user_data Additional poiner to User data
 */
function teoEventCb(ke, ev, data, data_len, user_data) {
    var rd;

    switch (ev) {
        
        // EV_K_STARTED #0 Calls immediately after event manager starts
        case teonet.ev.EV_K_STARTED:
            _ke = ke;
            console.log('Teoloadsi-js started .... ');
            break;

        // EV_K_CONNECTED #3 New peer connected to host event
        case teonet.ev.EV_K_CONNECTED:
            rd = new teonet.packetData(data);
            console.log('Peer "' + rd.from + '" connected');
            peers[rd.from] = 0;
            break;

        // EV_K_DISCONNECTED #4 A peer was disconnected from host
        case teonet.ev.EV_K_DISCONNECTED:
            rd = new teonet.packetData(data);
            console.log('Peer "' + rd.from + '" disconnected'/*, arguments*/);

            delete peers[rd.from];
            break;

        // EV_K_TIMER #9 Timer event, seted by ksnetEvMgrSetCustomTimer
        case teonet.ev.EV_K_TIMER:
            
            //test(_ke);
            
            if(start_f) {

                console.log("Client mode, server name:", SERVER_PEER);
                id = 0;
                sendCmdT(ke, SERVER_PEER, ++id, 0);
                
                //test_sendCmdAnswerToBinary(ke);
                //test_subscribe(ke);

                start_f = 0;
            } 
            else sendRandom(NUM_RUNDOM);
            
            break;

        // EV_K_RECEIVED #5 This host Received a data
        case teonet.ev.EV_K_RECEIVED:
            rd = new teonet.packetDataPointer(data);

            // Command
            switch (rd.cmd) {

                case teoApi.CMD_T: {
                    let d = teonet.dataToBuffer(rd.data, rd.data_len);
                    d.type = cmdTData;
                    let dref = d.deref();

                    if(dref.id !== id) errors++;
                    if(!(dref.id%NUM_TO_SHOW))
                        console.log(" Got an answer", 
                            dref.id, `(type: ${dref.type})`,
                            errors ? `errors: ${errors}` : "");
                    if(!SEND_IN_MAIN) sendCmdT(ke, SERVER_PEER, ++id, 0);
                    else send_f = 1;

                    sendRandom(NUM_RUNDOM);

                    SEND_IN_MAIN = SEND_IN_MAIN ? 0 : 1;
                } break;

                case teoApi.CMD_ECHO_ANSWER:
                    peers[rd.from] = 0;
                    break;

                default:
                    break;
            }
            break;

        // EV_K_USER #11 User press A hotkey
        case teonet.ev.EV_K_USER:
            break;

        case teonet.ev.EV_K_STOPPED:
            break;

        default:
            break;
    }
}

/**
 * Initialize and start Teonet
 *
 * @returns {undefined}
 */
function teo_main() {

    // Initialize teonet event manager and Read configuration
    var ke = teonet.init(teoEventCb, 3);

    // Set application type
    teonet.setAppType(ke, "teo-load-si-js");

    // Set application version
    teonet.setAppVersion(ke, '0.0.1');

    // Start Timer event
    teonet.setCustomTimer(ke, 1.00);

    // Start teonet
    teonet.run(ke, () => {
        console.log('Bye!');
        process.exit(0);
    });

    // Show exit message
    console.log("Teoloadsi-js application initialization finished ...");
}

function sender() {


    setTimeout(()=>{

        if(_ke !== undefined) {
            if(send_f) {
                sendCmdT(_ke, SERVER_PEER, ++id, 0);
                sendRandom(NUM_RUNDOM*20);
            }
            //console.log("main, send id:", id);
        }
        sender();

    }, 0);

}

if(SEND_IN_MAIN) sender();
