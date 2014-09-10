$set noon
$define sys sys$library
$define x11 decw$include
$!
$write sys$output "Compiling..."
$cc new.c:r
$cc niff.c:r
$cc value.c:r
$cc zio.c:r
$cc cmuwmraster.c:r
$cc faces.c:r
$cc fbm.c:r
$cc gif.c:r
$cc imagetypes.c:r
$cc img.c:r
$cc mac.c:r
$cc mcidas.c:r
$cc mc_tables.c:r
$cc pbm.c:r
$cc pcx.c:r
$cc pdsuncomp.c:r
$cc rle.c:r
$cc rlelib.c:r
$cc sunraster.c:r
$cc vff.c:r
$cc vicar.c:r
$cc xbitmap.c:r
$cc xpixmap.c:r
$cc xwd.c:r
$cc bright.c:r
$cc clip.c:r
$cc compress.c:r
$cc dither.c:r
$cc fill.c:r
$cc halftone.c:r
$cc merge.c:r
$cc reduce.c:r
$cc rotate.c:r
$cc smooth.c:r
$cc undither.c:r
$cc zoom.c:r
$cc config.c:r
$cc misc.c:r
$cc options.c:r
$cc root.c:r
$cc send.c:r
$cc window.c:r
$cc xloadimage.c:r
$cc vms-build.c
$!
$write sys$output "Linking..."
$link xloadimage, -
        new.o:r, -
        niff.o:r, -
        value.o:r, -
        zio.o:r, -
        cmuwmraster.o:r, -
        faces.o:r, -
        fbm.o:r, -
        gif.o:r, -
        imagetypes.o:r, -
        img.o:r, -
        mac.o:r, -
        mcidas.o:r, -
        mc_tables.o:r, -
        pbm.o:r, -
        pcx.o:r, -
        pdsuncomp.o:r, -
        rle.o:r, -
        rlelib.o:r, -
        sunraster.o:r, -
        vff.o:r, -
        vicar.o:r, -
        xbitmap.o:r, -
        xpixmap.o:r, -
        xwd.o:r, -
        bright.o:r, -
        clip.o:r, -
        compress.o:r, -
        dither.o:r, -
        fill.o:r, -
        halftone.o:r, -
        merge.o:r, -
        reduce.o:r, -
        rotate.o:r, -
        smooth.o:r, -
        undither.o:r, -
        zoom.o:r, -
        config.o:r, -
        misc.o:r, -
        options.o:r, -
        root.o:r, -
        send.o:r, -
        window.o:r, -
        vms-build.o, -
        sys$input:/options
sys$share:decw$xlibshr/share
$!
$exit
