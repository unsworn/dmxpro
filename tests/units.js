module.exports={
    assert: function(cond) {
        if (!cond) {
            process.stdout.write('[31m[1m ASSERTION FAILED\n[0m');
            throw new Error('[35m[1mTest has failed\n[0m');
        }
    },
    run: function(name, func) {
        process.stdout.write('[37m[1mtest(' + name + ')[0m');
        process.stdout.write('...');
        func();
        process.stdout.write('[32m[1m OK\n[0m');
    }
};
