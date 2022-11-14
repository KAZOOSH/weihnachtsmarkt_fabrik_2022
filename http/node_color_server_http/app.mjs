import express from "express";
import chroma from "chroma-js";
import { WebSocketServer } from 'ws';
const udp = require('dgram');

// todo: write udp stuff to send message to connected wifi led

const colorScale = chroma.scale('Spectral').domain([0,100]);

const app = express();app.listen(3000, () => {
 console.log("Server running on port 3000");
});

const wsServer = new WebSocketServer({port: 3030});


app.use(express.static("public"));

const xmasBalls = new Map();
let globalColorScaleValue = 0; // 0-100 on color scale

wsServer.on('connection', (socketClient) => {
    socketClient.send(JSON.stringify(getMessageForFrontend()));

    socketClient.on('message', (message) => {
        const jsonMessage = JSON.parse(message.toString());
        console.log(jsonMessage)
        if(jsonMessage.globalColor) {
            globalColorScaleValue = jsonMessage.globalColor;
            onUpdateGlobalColorValue();
        } else {
            const ball = xmasBalls.get(jsonMessage.id);
            if(ball) {
                ball[jsonMessage.color] = parseInt(jsonMessage.value);
                xmasBalls.set(jsonMessage.id, ball)
            }
            console.log(xmasBalls)
        }

    })
})

const getMessageForFrontend = () => {
    //console.log('getMessageForFrontend', xmasBalls)
    return {
    globalColorScaleValue: globalColorScaleValue,
    balls: [...xmasBalls]
}}

const updateFrontend = () => {
    wsServer.clients.forEach(client => client.send(JSON.stringify(getMessageForFrontend())));
}

const interval = setInterval(() => {
    // console.log('check activity')
    let keysToDelete = [];
    xmasBalls.forEach((value,key,map) => {
        const time = new Date().getTime();
        //console.log(time, value.lastRequest, time - value.lastRequest)
        if(time - value.lastRequest > 4000) {
            keysToDelete.push(key);
        }
    })
    keysToDelete.forEach(((key) => {
        console.log('delete ',key);
        xmasBalls.delete(key);
        updateFrontend();
    }))
}, 1000)

const onUpdateGlobalColorValue = () => {
    const chromaColor = colorScale( globalColorScaleValue || 50);
    const rgbColor = chromaColor.rgb();
    console.log('rgbColor', rgbColor)
    xmasBalls.forEach((value,key,map) => {
        map.set(key, {r: rgbColor[0], g: rgbColor[1], b: rgbColor[2]})
    })
    updateFrontend();
}

app.get("/setGlobalColor", (req, res, next) => {
    globalColorScaleValue = req.query.value;
    onUpdateGlobalColorValue();
    res.json({globalColorValue: `${globalColorScaleValue}`});
});


app.get("/color", (req, res, next) => {
        console.log(req.query.id, new Date())
		console.log('from ip:', req.ip)
        // if no ball with this id is in map
        if(!xmasBalls.has(req.query.id)){
            console.log('add ',req.query.id);
            xmasBalls.set(req.query.id,{r:100, g:100, b: 100})
            updateFrontend();
        }
        const ball = xmasBalls.get(req.query.id);
        if(ball) {
            ball.lastRequest = new Date().getTime();
            xmasBalls.set(req.query.id, ball);
        }


        //console.log([ball.r/100,ball.g/100,ball.b/100,1])
        res.json([ball.r/255,ball.g/255,ball.b/255,1]);
   });
