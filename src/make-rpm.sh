#!/bin/sh

PKG="openlmi-networking"

dir="$(dirname $(realpath $0))"

"$dir/make-release.sh" $1 || exit 1

if [ $# -lt 1 ];
then
    # We want current dirty copy
    tag="--dirty"
else
    tag="$1"
fi

VERSION="$(git describe $tag | sed 's/-/_/g')"

tempdir="$(mktemp -d)"
mkdir -p "$tempdir"
trap "rm -rf $tempdir" EXIT

sed "s/^Version:.*$/Version: $VERSION/g" "$dir/openlmi-networking.spec" > "$tempdir/openlmi-networking.spec"
rpmdev-bumpspec -c "Version $VERSION" "$tempdir/openlmi-networking.spec"

rpmbuild --define "_sourcedir $PWD" -ba "$tempdir/openlmi-networking.spec"
