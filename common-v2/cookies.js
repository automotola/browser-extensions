forge['cookies'] = {
    'get': function(url, name, success, error) {
        internal.priv.call("cookies.get", {
            url: url,
            name: name
        }, success, error);
    },
    'remove': function(url, name) {
        internal.priv.call("cookies.remove", {
            url: url,
            name: name
        });
    }
};
