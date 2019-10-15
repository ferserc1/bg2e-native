
module.exports = {

    install: function() {
        const { exec } = require('child_process');
        function execCmd(cmd) {
            console.log("Executing: " + cmd);
            return new Promise((resolve,reject) => {
                exec(cmd, (err,stdout,stderr) => {
                    if (err) {
                        reject(err);
                    }
                    else {
                        console.log(cmd + ": done");
                        resolve();
                    }
                });
            })
        }

        return Promise.all([
            execCmd('npm install')
        ]);
    }
}