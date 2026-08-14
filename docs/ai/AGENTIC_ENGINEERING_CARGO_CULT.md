# Agentic engineering feels like a cargo cult.

After months of persistent effort, I still keep running into the same problem with AI coding.

I can use a careful workflow. I can break the work into small, exacting prompts. I can separate design from implementation and audit. I can run compilers, linters, type checks, mutation tests, property-based tests, and a large regression suite. I can tighten the spec until it seems almost impossible to misunderstand.

And still, multiple trivial, unnecessary errors keep getting through.

Usually they are small. Slightly wrong conditions. Corrections that create fresh mistakes nearby, or perhaps mistakes that only become visible nearby because that is where I look most closely.

That is why the failures are so frustrating. They are small, unnecessary, and repetitive.

The basic problem I have learned is simple: software engineering is deterministic, while language models are probabilistic.

Serious software is closer to following exact instructions than to having a conversation. The order matters. The details matter. One missing step can spoil the result. A language model is like something that has read thousands of instruction manuals and can write a convincing set of instructions from memory. The weakness appears when you actually try to follow them.

For serious software, the visible behaviour has to be right. In my case that means the UI and UX. Prompts have to be right. Key paths have to be right. State transitions have to be right. Edge conditions have to be right. Small mistakes count in full.

I have found that this becomes clearer as the process gets more disciplined. It is tempting to think that more scaffolding will solve it. More tests. More prompts. More agent roles. More auditing. More iteration. Those things have only served to expose the limit more clearly. The workflow can look rigorous while still depending on repeated probabilistic guesses.

It copies the visible form of engineering: roles, review loops, validation gates, and carefully staged hand-offs. From the outside, it resembles a serious development process. Apparently it produces something useful. With enough persistence, enough checking, and enough correction, it can yield an approximation that is good enough to keep.

A cargo cult never works. A radio made of palm fronds will never transmit, however carefully it is adjusted. Agentic engineering is not quite that hopeless. It does sometimes produce something usable. But that only happens because a human keeps supplying the judgement and checking that the ritual itself cannot provide.

You go through the right motions and you do get results. The trouble is that the results keep needing rescue. Trivial faults continue to appear. Much of the apparent discipline has been developed to compensate for the weakness of the underlying tool.
