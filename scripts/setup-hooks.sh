#!/bin/bash
# Setup script to configure git hooks for the project

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}Setting up git hooks for hyprlax...${NC}"

# Configure git to use our hooks directory
git config core.hooksPath .githooks

echo -e "${GREEN}✓ Git hooks configured!${NC}"
echo ""
echo "The following hooks are now active:"
echo "  - pre-commit: Runs linting checks before each commit"
echo ""
echo "To bypass hooks temporarily, use: git commit --no-verify"
echo "To disable hooks, run: git config --unset core.hooksPath"