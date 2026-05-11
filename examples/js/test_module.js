import {GTime, TimeType} from "gany:Gx";

function sayHi(user) {
    let time = new (GTime._toJsClass())(TimeType.SystemClock);
    // 等价：
    // let time = GTime.new(TimeType.SystemClock);
    console.log(`Hello, ${user}, [${time.toString("HH:mm:ss.zzz")}]!`);
}

function sayBye(user) {
    console.log(`Bye, ${user}, [${GTime.currentSystemTime().toString("HH:mm:ss.zzz")}]!`);
}

export {sayHi, sayBye};