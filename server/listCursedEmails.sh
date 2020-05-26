
# note that this doesn't cover all cases of cursing people, with .ru emails or
# whatever
# a recent test showed this hit all but 48 curses out of 3000+

./curseDBDump | grep "@" | sed "s/ .*//" | sed "s/.com\([0-9a-z]\)/.com \1/i" | sed "s/.co.uk\([0-9a-z]\)/.co.uk \1/i" | sed "s/.org.uk\([0-9a-z]\)/.org.uk \1/i" | grep " " | sed "s/.* //" | sort | uniq -c | sort -n
