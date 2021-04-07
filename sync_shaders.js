const { readdirSync, unlinkSync } = require("fs");
const { join } = require("path");
const { exec } = require("child_process");
const { homedir } = require("os");

const SHADER_DIRECTORY = join(__dirname, "data", "shaders");
const GLSLC = join(homedir(), "dev", "lib", "VulkanSDK", "1.2.162.1", "Bin32", "glslc.exe");

function get_dir_entities(dir) {
    let dir_entities = readdirSync(dir, { withFileTypes: true });

    return {
        files: dir_entities.filter(dir_entity => dir_entity.isFile()).map(file => file.name),

        subdirs: dir_entities.filter(dir_entity =>
            dir_entity.isDirectory()).map(subdir => join(dir, subdir.name)),
    };
}

// Clear all SPIR-V shaders.
function clear_spirv_files(dir) {
    let dir_entities = get_dir_entities(dir);

    dir_entities.files
        .filter(file => file.endsWith(".spv"))
        .map(spirv_file => join(dir, spirv_file))
        .forEach(spirv_file_path => unlinkSync(spirv_file_path));

    dir_entities.subdirs.map(clear_spirv_files);
}

// Compile new SPIR-V shaders.
function process_directory(dir) {
    let dir_entities = get_dir_entities(dir);

    dir_entities.files
        .map(shader_src => ({ shader_src: join(dir, shader_src), output: join(dir, `${shader_src}.spv`) }))
        .forEach(cmd_data => {
            let cmd = `"${GLSLC}" "${cmd_data.shader_src}" -o "${cmd_data.output}"`;
            exec(cmd, (err, stdout, stderr) => {
                console.log(cmd);
                if (err)
                    console.error(`\x1b[31m${err.message}\x1b[0m`);
            });
        });

    dir_entities.subdirs.map(process_directory);
}

clear_spirv_files(SHADER_DIRECTORY);
process_directory(SHADER_DIRECTORY);
