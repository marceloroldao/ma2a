# Memoria.ia ↔ MA2A Integration Boundary

Status: architectural integration note

Purpose: preserve the boundary between the Memoria.ia memory engine and the MA2A protocol while the Memoria.ia experimental lineage is consolidated toward v1.0.

This document records which capabilities currently explored inside Memoria.ia should remain memory-engine responsibilities and which capabilities should be integrated into MA2A instead of becoming permanent responsibilities of the Memoria.ia core.

## Architectural principle

Memoria.ia owns memory semantics.

MA2A owns distributed identity, trust, authorization, synchronization and network governance.

The intended boundary is:

```text
Application / local LLM
        |
    Memoria.ia
        |
  MA2A Adapter
        |
       MA2A
   /     |      \
 L1      L2      L3
```

Memoria.ia must remain usable without MA2A. MA2A may use Memoria.ia as its memory/state engine, but the memory engine must not require network PKI or federation governance to function locally.

## Capabilities that remain in Memoria.ia

The following belong to the memory engine and should remain available independently of MA2A:

- deterministic memory storage and retrieval;
- hierarchical temporal memory;
- memory lifecycle, consolidation, deconsolidation and reactivation;
- contradiction and reinforcement handling;
- namespace isolation;
- structural evidence graph;
- bounded structural inference over explicit source-backed edges;
- temporal structural state and historical queries;
- provenance metadata;
- confidence metadata and conservative confidence gates;
- independent evidence-origin corroboration;
- source reliability learned from explicit external adjudication;
- protection against circular reliability/adjudication dependencies;
- abstraction, relation and ontology-oriented memory structures;
- persistence backend abstraction, including BDR and SQLite;
- local application credential/isolation mechanisms required by the standalone Memoria.ia product;
- audit data necessary to explain why a memory result was returned.

These capabilities operate on memory evidence and should not depend on a distributed trust network.

## Capabilities that migrate to MA2A

The following concepts were useful experiments inside the Memoria.ia lineage, but their permanent architectural home should be MA2A.

### 1. Distributed authority identity

Source experiment: Memoria.ia v1.17/v1.18 lineage.

MA2A responsibility:

- organization identity;
- node identity;
- agent/device identity;
- authority/root-of-control identity;
- mapping several nodes/origins to one controlling authority;
- trusted root configuration.

Memoria.ia may store the resulting identity identifiers as provenance, but must not define the network trust hierarchy.

### 2. Authority attestations

Source experiment: v1.18.

MA2A responsibility:

- trusted issuer registry;
- attestation issuance;
- attestation verification;
- certificate/attestation identifiers;
- signature verification;
- trust-root policy.

Memoria.ia receives only validated authority metadata through the MA2A Adapter.

### 3. Attestation lifecycle

Source experiment: v1.19.

MA2A responsibility:

- validity windows;
- expiration;
- revocation;
- replay protection for revocation operations;
- network policy epoch or equivalent monotonic trust-policy version.

Memoria.ia may preserve historical trust metadata attached to evidence, but should not be the authority that decides network credential validity.

### 4. Authority key rotation

Source experiment: v1.20.

MA2A responsibility:

- cryptographic key ownership;
- key rotation;
- continuity proof;
- prevention of cross-authority key reuse;
- active/historical key resolution;
- handshake use of current valid keys.

Memoria.ia should never need private keys in order to perform memory retrieval.

### 5. Key compromise recovery

Source experiment: v1.21.

MA2A responsibility:

- compromised-key quarantine;
- emergency credential recovery;
- recovery evidence validation;
- recovery-key activation;
- post-recovery key continuity.

The Memoria.ia core may retain audit records of the event if supplied by MA2A, but it must not implement the cryptographic recovery authority.

### 6. Recovery quorum

Source experiment: v1.22.

MA2A responsibility:

- N-of-M recovery policy;
- authorized recovery approvers;
- approval uniqueness;
- replay protection;
- quorum evaluation for identity/key recovery.

This is network/organizational governance, not memory semantics.

### 7. Guardian independence

Source experiment: v1.23.

MA2A responsibility:

- guardian identities;
- guardian controller/domain ownership;
- independence counting by controlling authority rather than raw node count;
- anti-Sybil constraints for recovery governance.

Memoria.ia may expose memory evidence that supports a governance decision but should not own the guardian trust model.

### 8. Controller transfer

Source experiment: v1.24.

MA2A responsibility:

- transfer of guardian/node/organizational control;
- effective policy epoch;
- authorization by current controller;
- transfer evidence identifiers;
- replay protection;
- historical controller ownership.

### 9. Controller-transfer quorum

Source experiment: v1.25.

MA2A responsibility:

- independent approval policy for controller transfer;
- approver-controller-domain counting;
- threshold evaluation;
- binding approvals to the exact transfer tuple.

### 10. Controller-transfer approval revocation

Source experiment: v1.26.

MA2A responsibility:

- approval revocation;
- original-approver authorization;
- revocation replay protection;
- revocation timing relative to effective epoch;
- final quorum recalculation before transfer execution.

## What should be reused rather than discarded

Migration does not mean deleting the Memoria.ia experiments.

The v1.17-v1.26 code and tests are valuable executable specifications. They should be treated as behavioral reference material for MA2A implementation.

Recommended migration process for each capability:

1. preserve the Memoria.ia experimental module and tests unchanged as historical evidence;
2. extract the behavioral contract into an MA2A specification section;
3. implement the equivalent MA2A protocol/state-machine behavior;
4. port the relevant negative/replay/temporal tests into MA2A conformance tests;
5. expose the result back to Memoria.ia through a narrow adapter;
6. only then remove any runtime dependency on the experimental governance class from the Memoria.ia product path.

## MA2A Adapter contract

The Memoria.ia side should consume validated protocol metadata, not perform network trust decisions.

A future adapter should expose concepts similar to:

```text
MA2AIdentityContext
- organization_id
- node_id
- authority_id
- controller_id
- credential_id
- policy_epoch
- scopes
- trust_status

MA2AEvidenceContext
- source_node_id
- source_authority_id
- message_id
- trajectory/state version
- received_epoch
- provenance
- integrity_status

MA2AAuthorizationDecision
- permitted
- reason_code
- policy_epoch
- authority_id
- scopes
```

These are logical interface concepts, not a frozen wire-format definition.

Memoria.ia may use this metadata for namespace selection, provenance, corroboration, reliability gates and auditability.

## Important semantic separation

Two kinds of independence must remain distinct:

### Evidence independence — Memoria.ia

Question: "Did this fact/memory receive support from genuinely independent evidence origins?"

Used for retrieval/inference confidence.

### Authority independence — MA2A

Question: "Are these approvals controlled by genuinely independent organizational/security authorities?"

Used for recovery, control transfer, licensing and federation governance.

They can share identifiers through the adapter, but they must not be treated as the same algorithmic concept.

## Product boundary

Standalone mode:

```text
Application -> Memoria.ia -> local persistence
```

No MA2A identity infrastructure is required.

MA2A-enabled mode:

```text
Application
    -> Memoria.ia
        -> MA2A Adapter
            -> authenticated MA2A state exchange
```

The adapter augments provenance/trust metadata but does not replace the memory engine.

## Integration order

Recommended order for MA2A implementation:

1. identity model: organization, node, authority, controller;
2. key/certificate and attestation model;
3. handshake and scope authorization;
4. lifecycle: expiry and revocation;
5. key rotation and compromise recovery;
6. guardian/recovery quorum;
7. controller transfer/quorum/revocation;
8. state-delta transport and replay protection integration;
9. Memoria.ia adapter integration;
10. cross-repository conformance tests.

## Acceptance criteria for the boundary

The migration is considered architecturally complete when:

- Memoria.ia can run locally with no MA2A dependency;
- MA2A can authenticate identities and manage key/controller lifecycle without delegating trust decisions to Memoria.ia;
- Memoria.ia receives validated MA2A metadata through an adapter;
- evidence provenance remains traceable across the boundary;
- MA2A replay/identity/security tests are independent from Memoria.ia retrieval tests;
- standalone product behavior remains unchanged when MA2A is disabled;
- MA2A-enabled mode preserves namespace and organization isolation;
- no private network credential is required inside the Memoria.ia memory core.

## Source lineage

Behavioral source material currently lives in the Memoria.ia experimental lineage, especially the v1.17-v1.26 authority/governance modules and tests.

Those experiments should be considered migration inputs, not permanent evidence that all governance belongs inside the memory engine.
