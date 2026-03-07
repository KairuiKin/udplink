# Security Policy

## Scope

This project currently provides message integrity/authentication (SipHash-2-4) and anti-replay controls.
It does not provide payload confidentiality.

## Supported Security Features

- Session isolation (`session_id`)
- Anti-replay sliding window (`nonce`)
- Optional authentication (`auth_key0/auth_key1`, with legacy `auth_psk` fallback)
- Online key rotation and acknowledgement convergence

## Threat Model Notes

- This library is designed for constrained environments and is not a full TLS replacement.
- Deployments requiring confidentiality should add a secure outer transport (for example DTLS or tunnel encryption).

## Reporting Vulnerabilities

Please report vulnerabilities privately to the maintainers first.
Do not open public issues for unpatched security vulnerabilities.

When reporting, include:

1. Affected version/commit
2. Reproduction steps
3. Expected impact
4. Suggested mitigation (if available)
