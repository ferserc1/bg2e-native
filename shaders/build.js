const fs = require('fs');
const path = require('path');

const platforms = {
    "darwin": "osx",
    "win32": "win",
    "linux": "linux"
};

const config = require(__dirname + "/config");

if (!fs.existsSync(config.intermediatePath)) {
    fs.mkdirSync(config.intermediatePath);
}

const command = require(config.scriptsPath + 'command');

function addBuildCommands(name,shaderFilePath,type,buildCommands = []) {
    let profiles = [
        '120'   // OpenGL
    ];

    function addProfiles(platform) {
        config.profiles[platform][type].forEach((p) => {
            if (profiles.indexOf(p) == -1) {
                profiles.push(p);
            }
        });
    }

    addProfiles('common');
    addProfiles(process.platform);

    profiles.forEach((profile) => {
        let arrayName = `${ name }_${ type }_${ profile }`;
        let outFile = `${ arrayName }.bin.h`;
        let outPath = `${ config.intermediatePath }/${ outFile }`;
        
        let command = `${ config.shaderc } ` +
            `-f ${ shaderFilePath } ` +
            `-o ${ outPath } ` +
            `-i ${ config.shaderIncludePath } ` +
            `--type ${ type[0] } ` +
            `--platform ${ platforms[process.platform] } ` + 
            `-p ${ profile} ` +
            `--bin2c ${ arrayName }`;
        
        buildCommands.push(command);
    });

    return buildCommands;
}

// Generate command list
let buildCommands = [];
fs.readdirSync(config.shadersPath).forEach((shaderDir) => {
    let shaderPath = path.join(config.shadersPath,shaderDir);
    fs.readdirSync(shaderPath).forEach((shaderFileName) => {
        if (/\.fs\./.test(shaderFileName)) {
            addBuildCommands(shaderDir,path.join(shaderPath,shaderFileName),'fragment',buildCommands);
        }
        else if (/\.vs\./.test(shaderFileName)) {
            addBuildCommands(shaderDir,path.join(shaderPath,shaderFileName),'vertex',buildCommands);
        }
    });
});

// Execute commands
let promises = [];
buildCommands.forEach((buildCommand) => {
    promises.push(
        new Promise((resolve,reject) => {
            command.exec(buildCommand)
                .then((stdout) => {
                    resolve();
                })
                .catch((err) => {
                    console.error(err.message);
                    resolve();
                });
        })
    );
});

Promise.all(promises)
    .then(() => {
        // Create the include file from the template and the intermediate files
        console.log("Intermediate files build complete. Generating shader C header...");

        let buffer = "";
        fs.readdirSync(config.intermediatePath).forEach((intermediateFile) => {
            let intermediateFilePath = path.join(config.intermediatePath,intermediateFile);
            buffer += fs.readFileSync(intermediateFilePath, { encoding: 'utf-8'}) + "\n\n";
        });

        let headerFile = fs.readFileSync(config.outFileTemplate,{ encoding: 'utf-8' });
        headerFile = headerFile.replace("%SHADER_ARRAYS%",buffer);
        fs.writeFileSync(config.outFile, headerFile);
    });



// command.exec('ls')
//     .then((result) => {
//         console.log(result.stdout);
//     });

