#!/usr/bin/env bash

project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
scripts_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
if [[ $scripts_dir == */scripts ]]; 
then
    #
    # Remove '/scripts' from the end of scripts_dir to get the soul directory
    #
    project_dir="${project_dir%/scripts}"
else
    scripts_dir="${scripts_dir}/scripts"
fi
if [[ -e "${scripts_dir}/bash-tools.sh" ]]; 
	then
    . "${scripts_dir}/bash-tools.sh"
	
	else
  echo -e "\nUnable to find required 'bash-tools.sh' in the expected location."
  echo -e "Searched Path: '$scripts_dir/bash-tools.sh'"
  set -e
  exit 1
fi

# Default options are:
build_dir="${project_dir}/${CMAKE_BINARY_DIR}"
build_option="${DEFAULT_BUILD_TARGET}"
cmake_generator="${DEFAULT_CMAKE_GENERATOR}"
parallel_jobs=2
target_name="notarius"

# Call if you need to check these paths
verify_paths()
{
  echo "${target_name}: ${project_dir}"
  echo "scripts: ${scripts_dir}"
  echo "build_dir: ${build_dir}"
  echo "build_option: ${build_option}"
  echo "cmake_generator_option: ${cmake_generator}"
}
verify_paths
echo parallel_jobs

# Function to show the script usage.
show_usage() {
  echo -e "\n${i_yellow}Example Usage:${i_purple} $0 ${i_white}{Optional Args: ${i_purple}-c -b ${i_white}{config}${i_purple} -t ${i_white}{target_name}${i_white}}${i_def}" \
          "\n  -h or --help                    ${i_green}# Display help documentation.${i_def}" \
          "\n  -c or --clean                   ${i_green}# Removes the '${build_dir}' directory${i_def}" \
          "\n  -b or --build {build_option}    ${i_green}# Use to define Debug, Release, MinSizeRelease, etc.${i_def}"\
          "\n  -t or --target {target_name}    ${i_green}# Defines the project name to build (default:${target_name}).${i_def}"\
          "\n  -j or --j {n}                   ${i_green}# Specifies the number of parallel jobs to be used when building the${i_def}\n"\
          "                                 ${i_green}# project (defaults to 0 which directs cmake to use all jobs avaliable).${i_def}"    
          
  echo -e "${i_yellow}Default Options:${i_def}\n  --config ${build_option}\n  --target ${target_name}\n  --cmake_generator ${cmake_generator}\n  -j ${parallel_jobs}\n"
}

# Parse the command-line arguments
#
while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--clean)
      rm -rf "${build_dir}"
      ;;
    -b|--build)
      build_option=$2
      shift
      ;;
    -t|--target)
      target_name=$2
      shift
      ;;
    -j|--parallel)
      parallel_jobs=$2
      shift
      ;;
    -h|--help)
      clear
      show_usage
      exit 1
      ;;
    *)
      #clear
      #echo "Invalid option: $1" >&2
      show_usage
      exit 1
      ;;
  esac
  shift
done


# If no clean argument is provided or an invalid argument is provided, continue with the build process
show_usage
sleep 2

if [[ ! -d ${build_dir} ]];
then
   mkdir -p "${build_dir}"
fi

cd "${build_dir}"

if os_windows;
then
   cmake ..
else
   cmake -DCMAKE_BUILD_TYPE="${build_option}" -G "${cmake_generator}" ..
fi

cmake --build . --config "${build_option}" -j "${parallel_jobs}"
