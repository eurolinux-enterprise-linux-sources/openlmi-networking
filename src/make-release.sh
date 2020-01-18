#!/bin/sh

PKG="openlmi-networking"

dir="$(dirname $(realpath $0))"

if [ $# -lt 1 ];
then
    printf "Warning: creating tarball of working copy is not reproducible\n" >&2
    # We want current dirty copy
    tag="--dirty"
else
    tag="$1"
fi

VERSION="$(git --git-dir=$dir/.git describe $tag | sed 's/-/_/g')"

if [ $? -ne 0 ];
then
    exit 2
fi

if [[ "$tag" != "--dirty" ]];
then
    # Making archive from git tag/commit, use git archive and inject pregenerated svgs
    archive="$(pwd)/$PKG-$VERSION.tar"
    git --git-dir="$dir/.git" archive --format=tar --prefix="$PKG-$VERSION/" "$tag" | gzip > "${archive}.gz"
else
    # Working copy is dirty, we have to copy the files manually
    tempdir="$(mktemp -d)"
    mkdir -p "$tempdir/$PKG-$VERSION"
    trap "rm -rf $tempdir" EXIT

    # This ugly thing should properly eat files with whitespace in the name
    git --git-dir="$dir/.git" ls-files | while IFS= read f;
    do
        mkdir -p "$(dirname $tempdir/$PKG-$VERSION/$f)";
        cp "$dir/$f" "$tempdir/$PKG-$VERSION/$f";
    done
    tar -czf "$PKG-$VERSION.tar.gz" -C "$tempdir" "$PKG-$VERSION/"
fi
printf "$PKG-$VERSION.tar.gz\n"
