#ifndef PATHS_H
#define PATHS_H

/* Where Lucy keeps everything, next to ai.exe:
 *
 *   mind_original/      the brain she was trained with and who she is
 *                       (start.web, start.voc, self.txt)
 *   mind/language/      her words: the lexicon, plain talk, verbs, limits
 *                       and sayings.txt, the sentences she says herself
 *   mind/memory/        knowledge packages (.mem) and catalog.txt
 *   mind/memory/general what she learned from everyone (facts, taught
 *                       facts, formulas, her own copy of the brain)
 *   mind/memory/persons what each person told her about themselves
 *   mind/memory/locals  each chat's notes, goal and history
 *   mind/memory/topics  topics saved with "export"
 *   mind/skills/        skill libraries (.dll on Windows, .so elsewhere)
 *   files/              the only folder she may change on the computer */

#define MIND_DIRECTORY "mind"
#define ORIGINAL_DIRECTORY "mind_original"
#define LANGUAGE_DIRECTORY "mind/language"
#define MEMORY_DIRECTORY "mind/memory"
#define GLOBAL_DIRECTORY "mind/memory/general"
#define PERSONS_DIRECTORY "mind/memory/persons"
#define LOCALS_DIRECTORY "mind/memory/locals"
#define TOPICS_DIRECTORY "mind/memory/topics"
#define OLD_KNOWLEDGE_DIRECTORY "mind/memory/old_knowledge"
#define SKILLS_DIRECTORY "mind/skills"
#define PACKAGES_DIRECTORY "mind/memory"
#define PACKAGES_SECOND_DIRECTORY "mind/memory/general"

#define LANGUAGE_PATH LANGUAGE_DIRECTORY "/words.lex"
#define PLAIN_TALK_PATH LANGUAGE_DIRECTORY "/plain_talk.txt"
#define SAFE_PATH LANGUAGE_DIRECTORY "/safe.txt"
#define LIMITS_PATH LANGUAGE_DIRECTORY "/limits.txt"
#define SAYINGS_PATH LANGUAGE_DIRECTORY "/sayings.txt"
#define VERBS_FILE_PATH LANGUAGE_DIRECTORY "/verbs.txt"
#define GLOBAL_WEB_PATH GLOBAL_DIRECTORY "/brain.web"
#define GLOBAL_VOCABULARY_PATH GLOBAL_DIRECTORY "/vocab.map"
#define START_WEB_PATH ORIGINAL_DIRECTORY "/start.web"
#define START_VOCABULARY_PATH ORIGINAL_DIRECTORY "/start.voc"
#define SELF_PATH ORIGINAL_DIRECTORY "/self.txt"

#endif
