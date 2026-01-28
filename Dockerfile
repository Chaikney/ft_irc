# syntax=docker/dockerfile:1

# taking "penultimate" to be the most-patched of the previous major version
ARG PENULTIMATE_VERSION=3.22.3
FROM alpine:${PENULTIMATE_VERSION} AS buildimg

# Install the alpine equivalent of build-essential for compiling C++ code
RUN ["apk", "add", "build-base"]

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY ./source source/
COPY ./include include/
COPY ./Makefile .

# Compile the C++ code statically to ensure it doesn't depend on runtime libraries
RUN make static

# Stage 2: Runtime stage
FROM scratch
LABEL author=chaikney
LABEL name=FT_IRC
LABEL description="A simple IRC server developed in C++"

# Copy the static binary from the build stage
COPY --from=buildimg /build/ircserv /ircserv

ARG	FT_PORT:3368
ENV FT_PORT=FT_PORT
ARG FT_PASS:"insecuredefault"

#TODO Could I add a HEALTHCHECK --interval=30s --timeout=10s --retries=3 CMD nginx -t || exit 1

# this can be overridden by docker run,
# but serves as useful documentation.
# TODO Change to use a variable that links to the CMD below
# FIXME Doesn't work
# EXPOSE ${FT_PORTARG}/tcp

# TODO Add a healthcheck
# HEALTHCHECK --interval=30s --timeout=10s --retries=3 CMD nginx -t || exit 1

# start ft_irc
# TODO password and port parameters should be handled properly
CMD ["./ircserv", $FT_PORT, $FT_PASS]
