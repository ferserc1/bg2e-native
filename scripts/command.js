
const { exec } = require('child_process');

module.exports = {
    exec: function(cmd,cwd="") {
        console.log("Executing: " + cmd);
        return new Promise((resolve,reject) => {
            let params = {};
            params.cwd = cwd;
            exec(cmd, cwd, (err,stdout,stderr) => {
                if (err) {
                    reject(err);
                }
                else {
                    console.log(cmd + ": done" + (cmd ? ` (${ cwd })` : ''));
                    resolve();
                }
            });
        });
    }
}