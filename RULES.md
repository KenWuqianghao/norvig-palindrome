# Rules (frozen 2026-11-17)

These rules copy Peter Norvig's 2016 palindrome rules. Do not change them during the hill-climb.

## Source

| Item | Value |
|---|---|
| Record page | https://www.norvig.com/palindrome.html (Version 3, 2016-06-10: 21,012 words, 90,439 letters, 16,111 phrases) |
| Program | https://github.com/norvig/pytudes/blob/main/ipynb/pal3.ipynb and `py/pal3.py` |
| Licence | MIT, "Copyright (c) 2010-2017 Peter Norvig" (`upstream/LICENSE`) |
| Phrase list | http://norvig.com/npdict.txt, 126,342 lines, 125,512 distinct letter keys |

SHA-256 of the files in `upstream/`:

```
3f28b8a95d92be6c2f73a63bfee0c80e6ad2718c1ee191b7032cdd5f09dbb51f  npdict.txt
c1e0dadff99963b2c784c0e3e0d808aef04b3dc5fc314cfcb5cd2ca6c6067d94  pal3.ipynb
c21a79f77e3021c098b4223e6f63976479733a699b3dc0bf4b33499a711758e2  pal3.py
dd325ad86b175663be94c76100eff3306bf6618d7537538616dd32e546da32c7  LICENSE
5dfab72d8e11371678ee506b987a19752ba1b420bd64dd29ee21e9c84bc41eb7  palindrome.html
```

## Rules

1. Each phrase is one line of `npdict.txt`, with the whitespace at the ends removed.
2. The key of a phrase is its letters in lowercase. Norvig's `letters()` removes `\W` characters. A phrase whose key has a digit or `_` is not allowed (about 20 phrases, for example `Blink-182`). This rule is stricter than Norvig's.
3. No two phrases in a palindrome have the same key. Norvig's program uses this rule (`self.set` holds keys). Thus "Ada" and "ADA" count as one phrase.
4. The palindrome starts with `a man`, `a plan` and ends with `a canal`, `Panama`. These 4 phrases count, as in Norvig's count.
5. The concatenation of all keys reads the same forward and backward. Spaces, case and punctuation do not count.
6. The center of the palindrome can be at any letter, also inside a phrase.

## Metric

- Primary: number of phrases (each one distinct by rule 3).
- Tie-break: number of words. A word is a whitespace-separated token of the phrase text, as Norvig counts it (`Donald E. Knuth` is 3 words; `non-American` is 1 word).

## Check

`python3 -I verify.py <file>`. The file has one phrase per line, in order. Norvig's text (`results/norvig_16111.txt`, from the output cell of `pal3.ipynb`) gives:

```
OK palindrome: 16,111 phrases, 21,012 words, 90,439 letters
```
