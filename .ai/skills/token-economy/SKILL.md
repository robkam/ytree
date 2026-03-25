---
name: usage-economy
description: Defensive protocols to prevent excessive usage allowance, quota, token, and context window burn-up.
---

# Usage Allowance & Economy Directives

You are operating under strict resource economy rules. Your system and user prompts consume usage allowance (also referred to as tokens, quota, computation, or context window) dynamically based on history depth and tool outputs.

You **MUST** follow these rules without exception to protect the user's usage allowance:

## 1. Mandatory Interception and Warnings
You **MUST** warn the user if they request an action that will consume an unnecessarily large amount of usage allowance. This applies broadly to ANY action that retrieves, generates, or parses massive amounts of text when a targeted approach exists.

## 2. Suggesting Economical Alternatives
Whenever you warn the user about a high-usage action, you **MUST** simultaneously provide the more economical alternative. You MUST always recommend the most targeted, precise, and lowest-context method available to achieve the user's goal.

## 3. Override Protocol
If you have warned the user and suggested an alternative, but the user explicitly instructs you to proceed with the expensive action anyway, you **MUST NOT** prevent them. Execute their explicit command.

## 4. Strict Semantic Tool Mandate
For the `ytree` codebase, you **MUST** ALWAYS default to specialized MCP semantic tools (**jCodeMunch** or **Serena**) over generic bash tools to save usage allowance. You MUST use semantic discovery, AST node retrieval, and structural search instead of raw file reading, generic grepping, or unstructured directory listings. 

## 5. Pruning and Pagination
You **MUST** paginate lists and searches unless explicitly instructed otherwise. If a conversation shifts to a completely different sub-system or topic, you **MUST** suggest the user opens a new chat session to drop the old context baggage.
