#!/bin/sh
set -e -x

SUPPORT=$HOME/.cache/support

install -d $SUPPORT

RELEASE_PATH=$TRAVIS_BUILD_DIR/configure/RELEASE_XXX.local
EPICS_BASE=$SUPPORT/epics-base
export RELEASE_PATH EPICS_BASE


# Helper functions
create_AXIS_RELEASE_PATH_local()
{
  file=$1 &&
  echo PWD=$PWD file=$file &&
  cat >$file <<EOF
EPICS_BASE  = $EPICS_BASE
SUPPORT     = $SUPPORT
EOF
}
  
create_AXIS_RELEASE_LIBS_local()
{
  file=$1 &&
  echo PWD=$PWD file=$file &&
  cat >$file <<EOF
ASYN        = $SUPPORT/asyn
EOF
}
  
create_DRIVERS_RELEASE_LIBS_local()
{
  file=$1 &&
  echo PWD=$PWD file=$file &&
  cat >$file <<EOF
ASYN        = $SUPPORT/asyn
AXIS        = $TRAVIS_BUILD_DIR
EOF
}


cat << EOF > $RELEASE_PATH
ASYN=$SUPPORT/asyn
EPICS_BASE=$SUPPORT/epics-base
EOF

if [ -n "$SEQ" ]; then
    echo SNCSEQ=$SUPPORT/seq >> $RELEASE_PATH
fi



# use default selection for MSI
#sed -i -e '/MSI/d' configure/CONFIG_SITE

if [ ! -e "$EPICS_BASE/built" ] 
then
    case "$BASE" in
    R3*)
      git clone --depth 10 --branch $BASE https://github.com/epics-base/epics-base.git $EPICS_BASE
      ;;
    R7*)
      git clone --recursive https://github.com/epics-base/epics-base.git $EPICS_BASE &&
        (
          cd $EPICS_BASE && git checkout $BASE 
        )
      ;;
    *)
      echo >&2 "Neither base 3.x nor base 7.x"
      exit 1
    esac
    EPICS_HOST_ARCH=`sh $EPICS_BASE/startup/EpicsHostArch`

    case "$STATIC" in
    static)
        cat << EOF >> "$EPICS_BASE/configure/CONFIG_SITE"
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
        ;;
    *) ;;
    esac

    case "$CMPLR" in
    clang)
      echo "Host compiler is clang"
      cat << EOF >> $EPICS_BASE/configure/os/CONFIG_SITE.Common.$EPICS_HOST_ARCH
GNU         = NO
CMPLR_CLASS = clang
CC          = clang
CCC         = clang++
EOF
      ;;
    *) echo "Host compiler is default";;
    esac

    # requires wine and g++-mingw-w64-i686
    if [ "$WINE" = "32" ]
    then
      echo "Cross mingw32"
      sed -i -e '/CMPLR_PREFIX/d' $EPICS_BASE/configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
      cat << EOF >> $EPICS_BASE/configure/os/CONFIG_SITE.linux-x86.win32-x86-mingw
CMPLR_PREFIX=i686-w64-mingw32-
EOF
      cat << EOF >> $EPICS_BASE/configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=win32-x86-mingw
EOF
    fi

    # set RTEMS to eg. "4.9" or "4.10"
    if [ -n "$RTEMS" ]
    then
      echo "Cross RTEMS${RTEMS} for pc386"
      install -d /home/travis/.cache
      curl -L "https://github.com/mdavidsaver/rsb/releases/download/travis-20160306-2/rtems${RTEMS}-i386-trusty-20190306-2.tar.gz" \
      | tar -C /home/travis/.cache -xj

      sed -i -e '/^RTEMS_VERSION/d' -e '/^RTEMS_BASE/d' $EPICS_BASE/configure/os/CONFIG_SITE.Common.RTEMS
      cat << EOF >> $EPICS_BASE/configure/os/CONFIG_SITE.Common.RTEMS
RTEMS_VERSION=$RTEMS
RTEMS_BASE=/home/travis/.cache/rtems${RTEMS}-i386
EOF
      cat << EOF >> $EPICS_BASE/configure/CONFIG_SITE
CROSS_COMPILER_TARGET_ARCHS+=RTEMS-pc386
EOF

    fi

    make -C "$EPICS_BASE" -j2
    # get MSI for 3.14
    case "$BASE" in
    3.14*)
        echo "Build MSI"
        install -d "$HOME/msi/extensions/src"
        curl https://www.aps.anl.gov/epics/download/extensions/extensionsTop_20120904.tar.gz | tar -C "$HOME/msi" -xvz
        curl https://www.aps.anl.gov/epics/download/extensions/msi1-7.tar.gz | tar -C "$HOME/msi/extensions/src" -xvz
        mv "$HOME/msi/extensions/src/msi1-7" "$HOME/msi/extensions/src/msi"

        cat << EOF > "$HOME/msi/extensions/configure/RELEASE"
EPICS_BASE=$EPICS_BASE
EPICS_EXTENSIONS=\$(TOP)
EOF
        make -C "$HOME/msi/extensions"
        cp "$HOME/msi/extensions/bin/$EPICS_HOST_ARCH/msi" "$EPICS_BASE/bin/$EPICS_HOST_ARCH/"
        echo 'MSI:=$(EPICS_BASE)/bin/$(EPICS_HOST_ARCH)/msi' >> "$EPICS_BASE/configure/CONFIG_SITE"

        cat <<EOF >> configure/CONFIG_SITE
MSI = \$(EPICS_BASE)/bin/\$(EPICS_HOST_ARCH)/msi
EOF

      ;;
    *) echo "Use MSI from Base"
      ;;
    esac

    touch $EPICS_BASE/built
else
    echo "Using cached epics-base!"
fi


# sequencer
if [ -z "$SEQ" ]; then
 :
elif [ ! -e "$SUPPORT/seq/built" ]; then
    echo "Build sequencer"
    install -d $SUPPORT/seq
    curl -L "http://www-csr.bessy.de/control/SoftDist/sequencer/releases/seq-${SEQ}.tar.gz" | tar -C $SUPPORT/seq -xvz --strip-components=1
    cp $RELEASE_PATH $SUPPORT/seq/configure/RELEASE
    make -C $SUPPORT/seq
    touch $SUPPORT/seq/built
else
    echo "Using cached seq"
fi


# asyn
if [ ! -e "$SUPPORT/asyn/built" ]; then
     echo "Build asyn"
     git clone --recursive https://github.com/epics-modules/asyn.git $SUPPORT/asyn &&
    (
        cd $SUPPORT/asyn && git checkout $ASYN
    )
    cp $RELEASE_PATH $SUPPORT/asyn/configure/RELEASE
    make -C "$SUPPORT/asyn" -j2
    touch $SUPPORT/asyn/built
else
    echo "Using cached asyn"
fi

# axis with submdules
echo "Build axis"
(
    git submodule init
    git submodule update
    (
      cd configure && 
      create_AXIS_RELEASE_PATH_local RELEASE_PATHS.local &&
      create_AXIS_RELEASE_LIBS_local RELEASE_LIBS.local
    )
    (
      cd axisCore &&
      make install  
    )
   for d in drivers/*; do
   (
      cd "$d" &&
      (
        echo SUB PWD=$PWD &&
        cd configure &&
        create_AXIS_RELEASE_PATH_local RELEASE_PATHS.local &&
        create_DRIVERS_RELEASE_LIBS_local RELEASE_LIBS.local
      ) &&
    make 
   )
   done
) 
