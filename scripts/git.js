

const fs = require('fs');

let execCmd = require('./command').exec;

module.exports = {
    clone: function(cloneUrl,dest) {
        return execCmd(`git clone ${ cloneUrl } ${ dest }`);
    },

    update: function(folder) {
        let promise = Promise.resolve();
        if (fs.existsSync(folder)) {
            console.log("Updating: " + folder);
            promise = execCmd(`git pull`, folder);
        }
        return promise;
    }
}
