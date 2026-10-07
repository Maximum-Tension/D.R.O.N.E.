#!/bin/sh
# builds ai (the chat), ai-train, ai-test and the skills in mind/skills
cd "$(dirname "$0")"
CORE="src/JIT.c src/WEB.c src/KERNEL.c src/REFERENCE.c src/LEARN.c src/LANGUAGE.c src/BRAIN.c src/THREAD.c src/DATA.c src/RECIPE.c"
mkdir -p mind/skills
gcc -O2 -o ai $CORE src/MEMORY.c src/CALCULATOR.c src/FILE_SYSTEM.c src/REFLEX.c src/NEAREST_NEIGHBOR.c src/KNOWLEDGE.c src/SENSE.c src/TAUGHT.c src/SOLVE.c src/AGENTS.c src/SKILLS.c src/PACKAGES.c src/SAYINGS.c src/WORK.c src/CHAT.c -lm -lpthread -ldl &&
gcc -O2 -o ai-train $CORE src/TRAIN.c -lm -lpthread &&
gcc -O2 -o ai-test $CORE src/TEST_KERNEL.c src/TEST_SHAPE.c src/TEST_RECIPE.c src/TEST_SIGNALS.c src/NEAREST_NEIGHBOR.c src/TEST_GROWTH.c src/TEST_TREE.c src/TEST_BRAIN.c src/KNOWLEDGE.c src/TEST_KNOWLEDGE.c src/CALCULATOR.c src/SENSE.c src/TEST_SENSE.c src/TEST_MAIN.c -lm -lpthread &&
for SKILL in src/skills/*.c; do
	NAME=$(basename "$SKILL" .c | tr 'A-Z' 'a-z')
	gcc -O2 -shared -fPIC -o "mind/skills/$NAME.so" "$SKILL" -lm || exit 1
done &&
echo done.
