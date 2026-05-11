(function (Env) {
    console.log("====== Task Begin ======");
    let v1 = Env.v1;
    let v2 = Env.v2;

    console.log("  Input v1:", v1);
    console.log("  Input v2:", v2);

    let v3 = v1 + v2;

    /**
     * @type {GThread}
     */
    const tGThread = GAny.import("Gx.GThread");
    tGThread.mSleep(2000);

    console.log("====== Task End ======");

    return v3;
})