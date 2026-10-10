FROM debian:bookworm-slim

ARG PSN00BSDK_VERSION=0.24
ARG PSN00BSDK_SHA256=5ada7c9478d795b22bde96ee23d6869290c14a740bcde3d57c01c342a26ded35

RUN apt-get update \
	&& apt-get install --yes --no-install-recommends \
		ca-certificates \
		cmake \
		curl \
		python3 \
		unzip \
	&& rm -rf /var/lib/apt/lists/*

RUN curl --fail --location --show-error \
		--output /tmp/psn00bsdk.zip \
		"https://github.com/Lameguy64/PSn00bSDK/releases/download/v${PSN00BSDK_VERSION}/PSn00bSDK-${PSN00BSDK_VERSION}-Linux.zip" \
	&& echo "${PSN00BSDK_SHA256}  /tmp/psn00bsdk.zip" | sha256sum --check \
	&& unzip -q /tmp/psn00bsdk.zip -d /tmp \
	&& mv "/tmp/PSn00bSDK-${PSN00BSDK_VERSION}-Linux" /opt/psn00bsdk \
	&& rm /tmp/psn00bsdk.zip

ENV PATH="/opt/psn00bsdk/bin:${PATH}"
ENV PSN00BSDK_LIBS="/opt/psn00bsdk/lib/libpsn00b"

WORKDIR /workspace
