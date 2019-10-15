
const http = require('http');
const fs = require('fs');
const path = require('path');
const unzip = require('unzipper');

// Path separators could change depending on the platform
function mkdirp(pathToCreate) {
    pathToCreate
        .split(path.sep)
        .reduce((currentPath, folder) => {
            currentPath += folder + path.sep;
            if (!fs.existsSync(currentPath)){
                fs.mkdirSync(currentPath);
            }
            return currentPath;
        }, '');
}

function deleteFolderRecursive(path) {
    if (fs.existsSync(path)) {
      fs.readdirSync(path).forEach(function(file, index){
        var curPath = path + "/" + file;
        if (fs.lstatSync(curPath).isDirectory()) { // recurse
          deleteFolderRecursive(curPath);
        } else { // delete file
          fs.unlinkSync(curPath);
        }
      });
      fs.rmdirSync(path);
    }
  };

function unzipFile(file,unzippedName="") {
    return new Promise((resolve,reject) => {

        let pathParsed = path.parse(file);
        let dstDir = pathParsed.dir;

        let error = false;
        fs.createReadStream(file)
            .pipe(unzip.Extract({ path: dstDir }))
            .on("close",function() {
                // Remove zip file
                fs.unlink(file, () => {
                    if (!error) resolve();
                });
            })
            .on("end",function() {
                resolve();
            })
            .on("error",function(err) {
                error = true;
                reject(new Error("could not unzip file " + file + ". " + message));
            });
    })
}

module.exports = {
    downloadFile: function(url,dest,unzippedName="") {
        return new Promise((resolve,reject) => {
            console.log(url + ' >> ' + dest);
            let file = fs.createWriteStream(dest);
            let request = http.get(url,(response) => {
                response.pipe(file);
                file.on('finish', () => {
                    file.close(() => {
                        console.log(`${ dest }: download finished.`);
                        if (path.parse(dest).ext=='.zip') {
                            console.log("Unzipping file.")
                            unzipFile(dest,unzippedName)
                                .then(() => {
                                    console.log("Unzip finished: " + dest);
                                    resolve()
                                })
                                .catch((err) => {
                                    console.error("Error unzipping file " + dest);
                                    reject(err);
                                })
                        }
                        else {
                            resolve()
                        }
                    })
                });
            });
            request.on("error",(err) => {
                console.error(`${ dest }: download error. ${ err.message }`);
                reject(err)
            });
        })
    },

    downloadFiles: function(files,dstFolder) {
        try {
            if (!fs.lstatSync(dstFolder).isDirectory()) {
                throw new Error("The destination directory is a file");
            }
        }
        catch(e) {
            if (e.code=='ENOENT') {
                // Create directory
                mkdirp(dstFolder);
            }
            else {
                throw e;
            }
        }

        let promises = [];

        files.forEach((fileData) => {
            let file = fileData.url;
            let dst = fileData.dest;
            promises.push(this.downloadFile(file,path.join(dstFolder,path.parse(file).base),dst));
        });
        return Promise.all(promises);
    }
}