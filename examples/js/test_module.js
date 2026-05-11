import {GTime} from "gany:Gx";

function sayHi(user) {
    console.log(`Hello, ${user}, [${GTime.currentSystemTime().toString("HH:mm:ss.zzz")}]!`);
}

function sayBye(user) {
    console.log(`Bye, ${user}, [${GTime.currentSystemTime().toString("HH:mm:ss.zzz")}]!`);
}

export {sayHi, sayBye};