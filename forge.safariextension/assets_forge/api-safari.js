/*
 * api-safari.js
 * 
 * Safari specific overrides to the generic Forge api.js
 */

var messageListeners = {},
	bridgeId = 'forge-bridge' + forge.config.uuid

window.__msg = messageListeners

safari.self.addEventListener("message", onMessage, false);

function onMessage(msgEvt) {
	if(msgEvt.name != bridgeId) return
	var msg = msgEvt.message
	if(msg.method == 'message') {
		return processMessage(msg)
	} 

	internal.priv.receive(msg)
}

function processMessage(msg) {
	var event = msg.event,
		data = msg.data

	var listeners = messageListeners[event]
	if(!listeners) return
	listeners.forEach(function(cb) {
		cb(data)
	})
}


internal.priv.send = function (data) {
	if (!safari.self.tab) return
	safari.self.tab.dispatchMessage(bridgeId, JSON.parse(JSON.stringify(data)))
};

// Override default implementations for some methods
forge.is.desktop = function () {
	return true;
};
forge.is.safari = function () {
	return true;
}


function listen(type, cb) {
	var listeners = messageListeners[type] = messageListeners[type] || []
	if(listeners.indexOf(cb) == -1) listeners.push(cb)
}

function unlisten(type, cb) {
	var listeners = messageListeners[type] = messageListeners[type] || [],
		i = listeners.indexOf(cb)
	if(i != -1) listeners.splice(i, 1)
	if(listeners.length == 0) delete messageListeners[type]
}

var safariBroadcast = function (event, data) {
	internal.priv.call("message", {event: event, data: data});
}

function sendToBackground(event, type, content, callback) {
	var id = forge.tools.UUID()
	var listener = function(data) {
		callback(data)
		unlisten(id, listener)
	}
	listen(id, listener)
	safariBroadcast(event, { type: type, content: content, id: id })
}

// Override default messaging methods
forge.message.listen = function (type, callback, error) {
	if (typeof(type) === 'function') {
	 // no type passed in: shift arguments left one
		error = callback;
		callback = type;
		type = null;
	}
	
	listen("broadcast", function(message) {
		if (type !== null && type !== message.type) return
			callback(message.content, function(reply) {
				if (!message.id) return
				safariBroadcast(message.id, reply)
			})
	})
}

forge.message.broadcastBackground = function (type, content, callback, error) {
	sendToBackground('broadcast-background', type, content, callback)
}

forge.message.broadcast = function (type, content, callback, error) {
	sendToBackground('broadcast', type, content, callback)
}

forge.message.toFocussed = function (type, content, callback, error) {
	sendToBackground('toFocussed', type, content, callback)
}

