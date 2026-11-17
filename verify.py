import hashlib, re, sys
DICT = 'upstream/npdict.txt'
assert hashlib.sha256(open(DICT, 'rb').read()).hexdigest().startswith('3f28b8a95d92be6c'), 'npdict.txt changed'
allowed = {l.strip() for l in open(DICT)}
key = lambda p: re.sub(r'\W+', '', p).lower()
phrases = [l.rstrip('\n') for l in open(sys.argv[1]) if l.strip()]
bad = [p for p in phrases if p not in allowed or not re.fullmatch(r'[a-z]+', key(p))]
assert not bad, f'not in npdict.txt or not letters-only: {bad[:5]}'
keys = [key(p) for p in phrases]
assert len(set(keys)) == len(keys), 'a phrase is used twice'
assert phrases[:2] == ['a man', 'a plan'] and phrases[-2:] == ['a canal', 'Panama'], 'wrong frame'
text = ''.join(keys)
assert text == text[::-1], 'not a palindrome'
print(f'OK palindrome: {len(phrases):,} phrases, {sum(len(p.split()) for p in phrases):,} words, {len(text):,} letters')
