# Provenance and licensing record

Machine-readable schema: `Content/IndianRailway/Production/Schemas/provenance.schema.json`.
Pending template: `Templates/provenance.json`. No acquisition is represented by it.
`SourceReference` accepts an original URL or repository/archive reference. AssetId,
AssetName, Category, Creator, Source, AcquisitionDate, License, CommercialUse,
ModificationPermission, RedistributionRestrictions, AttributionRequirement,
OriginalArchive, ModifiedAssetReference, VerificationStatus, VerificationEvidence and
Notes are required. An acquisition date may be null only before acquisition/review.

Workflow: unknown -> pending review -> approved / restricted / rejected. Restricted
rights require a later explicit review, never automatic release. Approval needs actual
license/source evidence, verified commercial and modification rights, acquisition date,
attribution/restriction text and an accountable owner. Store license/archive hashes or
immutable references alongside the evidence; do not redistribute an archive merely
because it is referenced. The validator checks declarations, not legal authenticity.

`validate_s8.py --release` rejects unresolved rights, unaccepted assets and the empty
foundation catalog. Foundation validation permits pending drafts, and that permission
does not authorize using those assets in a release. License grants never come from a
filename, a marketplace thumbnail or an AI-generated claim.
