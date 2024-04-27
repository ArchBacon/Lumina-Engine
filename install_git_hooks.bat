@echo off

REM Check if .git directory exists
IF NOT EXIST .git (
    echo ".git directory not found. Terminating script..."
    exit /b
)

REM If .git directory, exists, navigate inside
cd .git

REM Check if hooks directory exists inside .git
IF NOT EXIST hooks (
    REM If hooks directory doesn't exist inside .git, create the directory
    mkdir hooks
)

REM Navigate into hooks directory
cd hooks

REM Create post-checkout file and write command into it.
REM `post-checkout` runs after the `git checkout` or `git clone` commands
echo #!/bin/sh > post-checkout
echo exec ./generate_project_files.bat >> post-checkout

REM `post-merge` runs after the `git pull` command
echo #!/bin/sh > post-merge
echo exec ./generate_project_files.bat >> post-merge

REM `post-rewrite` runs after the `--amend` or `rebase` commands
echo #!/bin/sh > post-rewrite
echo exec ./generate_project_files.bat >> post-rewrite

echo "Script executed successfully"
