# Plan

The road to full draft 2020-12 support.

## Prerequisites

* ~~Update to latest version of JSON-Schema-Test-Suite~~
* Add `kDraft202012` parser mode

## Non-annotation keywords

* `$id` parsing (**in progress**)
* `$defs` alias/support
* `dependentRequired`
* `dependentSchemas`
* `prefixItems`
* `items` alternative behaviour for 2020
* `minContains`/`maxContains`, may be achievable before full annotation support

## References

* `$ref` as applicator with siblings
* schema registry and canonical URI handling
* `$anchor` support
* compound schema documents
* remote reference cache improvements
* dialect switching for embedded/referenced schemas

## Annotation-aware keywords

* `unevaluatedItems`
* `unevaluatedProperties`
* `contains` evaluated-index annotations
* annotation propagation

## Dynamic references

* `$dynamicAnchor`
* `$dynamicRef`
* dynamic-scope evaluation

## General

* `format` behaviours
