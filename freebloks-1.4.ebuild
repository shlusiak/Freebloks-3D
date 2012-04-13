# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils games
DESCRIPTION="Board game in 3D"
HOMEPAGE="http://saschahlusiak.de/blokus"
SRC_URI="http://saschahlusiak.de/blokus/${P}.tar.gz"
LICENSE="GPL-2"

SLOT="0"

KEYWORDS="~x86 ~amd64"
IUSE="X dedicated"

# Build-time dependencies, such as
#    ssl? ( >=dev-libs/openssl-0.9.6b )
#    >=dev-lang/perl-5.6.1-r1
# It is advisable to use the >= syntax show above, to reflect what you
# had installed on your system when you tested the package.  Then
# other users hopefully won't be caught without the right version of
# a dependency.
DEPEND=""

# Run-time dependencies, same as DEPEND if RDEPEND isn't defined:
#RDEPEND=""

src_compile() {
	egamesconf $(use_with dedicated) $(use_with X gui) || die "econf failed"
	emake || die "emake failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"
	if use X ; then
		newicon src/freebloks.png ${PN}.png
		make_desktop_entry freebloks Freebloks\ 3D ${PN}.png
	fi

	prepgamesdirs
}
