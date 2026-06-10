# Plan

The road to full draft 2020-12 support.

## Prerequisites

* ~~Update to latest version of JSON-Schema-Test-Suite~~
* ~~Add `kDraft202012` parser mode~~

## Non-annotation keywords

* ~~`$id` parsing~~
* ~~`$defs` alias/support~~
* `items` alternative behaviour for 2020 (**in progress**)
* `dependentRequired`
* `dependentSchemas`
* `prefixItems`
* `minContains`/`maxContains`, may be achievable before full annotation support

## General

* common `format` behaviours
* schema registry and canonical URI handling

## References

* `$ref` as applicator with siblings
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
