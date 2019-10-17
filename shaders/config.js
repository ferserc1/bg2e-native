
const platforms = {
    "darwin": "osx",
    "win32": "win",
    "linux": "linux"
};

module.exports = {
    scriptsPath: `${ __dirname }/../scripts/`,
    shadersPath: `${ __dirname }/src`,
    
    shaderc: `${ __dirname }/../deps/bgfx/${ platforms[process.platform] }/bin/shaderc`,
    outFileTemplate: `${ __dirname }/include/shaders.h.template`,
    outFile: `${ __dirname }/include/shaders.h`,
    intermediatePath: `${ __dirname }/include/intermediate`,
    shaderIncludePath: `${ __dirname }/lib`,

    profiles: {
        common: {
            vertex: [
                '120'
            ],
            fragment: [
                '120'
            ]
        },
        win32: {
            vertex: [
                'vs_5_0',
                'spirv'
            ],
            fragment: [
                'ps_5_0',
                'spirv'
            ]
        },
        darwin: {
            vertex: [
                'metal',
                'spirv'
            ],
            fragment: [
                'metal',
                'spirv'
            ]
        },
        linux: {
            vertex: [
                'spirv'
            ],
            fragment: [
                'spirv'
            ]
        }
    }
}