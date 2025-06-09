# === README.md ===
#2dsim08
Multithreaded creature sim in Qt/C++ by Claude Sonnet 4 June of 2025

I've been playing around with little creature simulations in Qt for years.  Some of them would eat plants that spawned occasionally and die if they didn't get fed.  Some would chase each other around.  Generally, I found that Qt could do logic for about 500 creatures in the main thread before it would start to slow down and the creatures movements would start to become jerky.  If the logic was very simple, it could do 1000 or so.  I always wanted to do a multithreaded version so I could have more creatures but I'm lazy--and can come up with a bunch more excuses besides "lazy" if required--most having to do with character defects and attitude problems and so forth.

So I never did it.  I fiddled around learning about Qt multithreading in an offhand way, but mutexes and critical sections are not as fun to work on as getting a dot-shaped cat to chase a bunch of dot-shaped mice.

But recently I tried this vibe coding thing, where I just let the LM write all the code.  I didn't write a lick.  I tweaked parameters, and I think I fixed one very small bug that I may have created myself when I changed some variable names.  Other than that, Claude wrote all the code.

It's less than 1,000 lines, so it's not big project, but it demonstrates some things well for anyone who... wants to try working on games in C++ on Qt... in 2025... lol.  Hey people are allowed to have weird hobbies what can I say.

So for anyone who hasn't tried to use these LMs to write code, or has tried and had difficulty, for example people who are just learning how to program, I thought I would include the conversation I had with Claude to get the code to the state is is in on 20250609.

I'm going to put the discussion between Claude and I in here first, and then I _may_ put copies of the code buffers in here, or links to them, I'm not sure.  That would add a bunch of time to do by hand, and I don't believe Claude has a convo export capability yet.