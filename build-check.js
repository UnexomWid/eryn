try {
    require('./build-load')('.');
    console.log('[CHECK] Build found');
    process.exit(0);
} catch {
    console.log('[CHECK] Build not found');
    process.exit(1);
}