#!/usr/bin/env sh

set -eu

# Validate all of the YAML files in the current repo

current_dir="$(cd "$(dirname "$0")"; pwd -P)"

python3 -m venv "${current_dir}/venv"
. "${current_dir}/venv/bin/activate"
python -m pip install 'pyyaml<=6.0.3' > /dev/null
git_root_dir="$(git rev-parse --show-toplevel)"
files="$(git -C "${git_root_dir}" ls-files '**.yml' '**.yaml')"
return_code=0

for file in ${files}
do
    full_path="${git_root_dir}/${file}"
    if ! python -c "import yaml, sys; yaml.safe_load(open('${full_path}'))"; then
        echo "FAILURE: File ${full_path} contains invalid YAML!"
        return_code=1
    else
        echo "SUCCESS: File ${full_path} is a valid YAML file."
    fi
done
exit ${return_code}
