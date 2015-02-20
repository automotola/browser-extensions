/*
 * api-chrome.js
 * 
 * Chrome specific overrides to the generic Forge api.js
 */


/**
 * Set up port through which all content script <-> background app
 * communication will happen
 */
var port = chrome.extension.connect({name: 'bridge'});
port.onMessage.addListener(function (msg) {
	internal.priv.receive(msg);
});


var bgPort = chrome.extension.connect({name:'message:to-priv'});
var messageListeners = {}, //{'someType': [listener1, ..]}
	callbacks = {}

bgPort.onMessage.addListener(function(message) {
		//console.log('got message from bg', message)
		//callbacks receive reply from bg
		if(callbacks[message.callid]) {
			//console.log('executed callback message')
			callbacks[message.callid](message.content)
			delete callbacks[message.callid]
			return
		}

		var listeners = messageListeners[message.type] || []
		for (var i = 0; i < listeners.length; i++) {
			listeners[i] && listeners[i](message.content, function(reply) {
				// send back reply
				//console.log('sent reply', {content: reply, callid: message.callid})
				bgPort.postMessage({content: reply, callid: message.callid}) //send reply to bg
			})
		}
})


/**
 * Calls native code from JS
 * @param {*} data Object to send to privileged/native code.
 */
internal.priv.send = function(data) {
	port.postMessage(data);
}

/**
 * @return {boolean}
 */
forge.is.desktop = function () {
	return true;
}

/**
 * @return {boolean}
 */
forge.is.chrome = function () {
	return true;
}

/**
 * Add a listener for broadcast messages sent by other pages where your extension is active.
 *
 * @param {string} type (optional) an arbitrary string: if included, the callback will only be fired
 *  for messages broadcast with the same type; if omitted, the callback will be fired for all messages
 * @param {function(*, function(*))=} callback
 * @param {function({message: string}=} error
 */



forge.message.listen = function(type, callback, error) {
	if (typeof type === 'function') {
		// no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}

	var listeners = messageListeners[type] = messageListeners[type] || []
	if(listeners.indexOf(callback)==-1) listeners.push(callback)

}
/**
 * Broadcast a message to listeners set up in your background code.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function(*)=} callback
 * @param {function({message: string}=} error
 */
forge.message.broadcastBackground = function(type, content, callback, error) { 
	var callid = forge.tools.UUID()
	if (callback) callbacks[callid] = callback
	//console.log('send message to background', {type: type, callid: callid, content: content})
	bgPort.postMessage({type: type, callid: callid, content: content});
};
/**
 * Broadcast a message to all other pages where your extension is active.
 *
 * @param {string} type an arbitrary string which limits the listeners which will receive this message
 * @param {*} content the message body which will be passed into active listeners
 * @param {function(*)=} callback
 * @param {function({message: string}=} error
 */
var broadcastPort = chrome.extension.connect({name:'message:to-non-priv'})
forge.message.broadcast = function(type, content, callback, error) {
	var callid = forge.tools.UUID()
	if(callback) callbacks[callid] = callback
	broadcastPort.postMessage({type: type, callid: callid, content: content});
};
