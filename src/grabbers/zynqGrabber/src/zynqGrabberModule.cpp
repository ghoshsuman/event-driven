// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Authors: Francesco Rea, Chiara Bartolozzi, Arren Glover
 * email:   francesco.rea@iit.it
 * website: www.robotcub.org
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

/**
 * @file zynqGrabberModule.cpp
 * @brief Implementation of the zynqGrabberModule (see header file).
 */

#include <iCub/zynqGrabberModule.h>

bool zynqGrabberModule::configure(yarp::os::ResourceFinder &rf) {


    std::string moduleName = rf.check("name", yarp::os::Value("zynqGrabber")).asString();
    setName(moduleName.c_str());
    bool strict = rf.check("strict") && rf.check("strict", yarp::os::Value(true)).asBool();




    if(rf.check("controllerDevice")) {

        std::string controllerDevice = rf.find("controllerDevice").asString();

        vsctrlMngLeft = vDevCtrl(controllerDevice, I2C_ADDRESS_LEFT);
        vsctrlMngRight = vDevCtrl(controllerDevice, I2C_ADDRESS_RIGHT);

        //bias values
        yarp::os::Bottle biaslistl = rf.findGroup("ATIS_BIAS_LEFT");
        yarp::os::Bottle biaslistr = rf.findGroup("ATIS_BIAS_RIGHT");

        bool con_success = false;

        if(!vsctrlMngLeft.setBias(biaslistl) || !vsctrlMngRight.setBias(biaslistr) ) {
            std::cerr << "Bias file required to run zynqGrabber" << std::endl;
            return false;
        }
        if(!vsctrlMngLeft.connect())
            std::cerr << "Could not connect to vision controller left" << std::endl;
        else
            if(!vsctrlMngLeft.configure(true)) {
                std::cerr << "Could not configure left camera" << std::endl;
            } else {
                con_success = true;
            }

        if(!vsctrlMngRight.connect())
            std::cerr << "Could not connect to vision controller right" << std::endl;
        else {
            if(!vsctrlMngRight.configure(true)) {
                std::cerr << "Could not configure right camera" << std::endl;
            } else {
                con_success = true;
            }
        }


        if(!con_success) {
            std::cerr << "A configuration device was specified but could not be connected" << std::endl;
            return false;
        }

    }



    //open rateThread device2yarp
    if(rf.check("dataDevice")) {

        std::string dataDevice = rf.find("dataDevice").asString();
        int readPacketSize = 8 * rf.check("readPacketSize", yarp::os::Value("128")).asInt();
        int maxBottleSize = 8 * rf.check("maxBottleSize", yarp::os::Value("100000")).asInt();

        if(!D2Y.initialise(moduleName, strict, dataDevice, readPacketSize, maxBottleSize)) {
            std::cout << "A data device was specified but could not be initialised" << std::endl;
            return false;
        } else {
            D2Y.start();
        }

        if(!Y2D.open(moduleName, dataDevice)) {
            std::cerr << "Could not open YARP ports for Y2D" << std::endl;
            return false;
        }
    }

    if (!handlerPort.open("/" + moduleName)) {
        std::cout << "Unable to open RPC port @ /" << moduleName << std::endl;
        return false;
    }
    attach(handlerPort);

    return true;
}

bool zynqGrabberModule::interruptModule() {
    handlerPort.interrupt();
    Y2D.interrupt();
    // D2Y ???
    return true;
}

bool zynqGrabberModule::close() {

    std::cout << "breaking YARP connections" << std::endl;
    handlerPort.close();        // rpc of the RF module
    Y2D.close();
    D2Y.stop();                // bufferedport from yarp to device

    std::cout << "closing device drivers" << std::endl;
    vsctrlMngLeft.disconnect(true);
    std::cout << "left controller closed" << std::endl;
    vsctrlMngRight.disconnect(true);
    std::cout << "right controller closed" << std::endl;

    return true;
}

/* Called periodically every getPeriod() seconds */
bool zynqGrabberModule::updateModule() {

    return true;
}

double zynqGrabberModule::getPeriod() {
    /* module periodicity (seconds), called implicitly by myModule */
    return 1.0;
}


// RPC
bool zynqGrabberModule::respond(const yarp::os::Bottle& command,
                                yarp::os::Bottle& reply) {
    bool ok = false;
    bool rec = false; // is the command recognized?
    std::string helpMessage =  std::string(getName().c_str()) +
            " commands are: \n" +
            "help \n" +
            "quit \n" +
            "set thr <n> ... set the threshold \n" +
            "(where <n> is an integer number) \n";

    reply.clear();

    if (command.get(0).asString()=="quit") {
        reply.addString("quitting");
        return false;
    }
    else if (command.get(0).asString()=="help") {
        std::cout << helpMessage;
        reply.addString("ok");
    }

    switch (command.get(0).asVocab()) {
    case COMMAND_VOCAB_HELP:
        rec = true;
    {
        reply.addString("many");
        reply.addString("help");

        reply.addString("");

        ok = true;
    }
        break;
    case COMMAND_VOCAB_SUSPEND:
        rec = true;
    {
        D2Y.suspend();
        ok = true;
    }
        break;
    case COMMAND_VOCAB_RESUME:
        rec = true;
    {
        D2Y.resume();
        ok = true;
    }
        break;
    case COMMAND_VOCAB_GETBIAS:
        rec = true;
    {
        std::string biasName = command.get(1).asString();
        std::string channel = command.get(2).asString();

        // setBias function
        if (channel == "left") {
            int val = vsctrlMngLeft.getBias(biasName);
            if(val >= 0) {
                reply.addString("Left: ");
                reply.addString(biasName);
                reply.addInt(val);
                ok = true;
            } else {
                reply.addString("Left Unknown Bias");
                ok = false;
            }
        } else if (channel == "right") {
            int val = vsctrlMngRight.getBias(biasName);
            if(val >= 0) {
                reply.addString("Right: ");
                reply.addString(biasName);
                reply.addInt(val);
                ok = true;
            } else {
                reply.addString("Right Unknown Bias");
                ok = false;
            }
        } else if (channel == "") {
            int val = vsctrlMngLeft.getBias(biasName);
            if(val >= 0) {
                reply.addString("Left: ");
                reply.addString(biasName);
                reply.addInt(val);
                ok = true;
            } else {
                reply.addString("Left Unknown Bias");
                ok = false;
            }
            val = vsctrlMngRight.getBias(biasName);
            if(val >= 0) {
                reply.addString("Right: ");
                reply.addString(biasName);
                reply.addInt(val);
                ok = true & ok;
            } else {
                reply.addString("RightUnknown Bias");
                ok = false;
            }
        }
        else {
            std::cout << "unrecognised channel" << std::endl;
            ok = false;
        }
    }
        break;

    case COMMAND_VOCAB_SETBIAS:
        rec = true;
    {
        std::string biasName = command.get(1).asString();
        unsigned int biasValue = command.get(2).asInt();
        std::string channel = command.get(3).asString();

        // setBias function
        if (channel == "left"){
            vsctrlMngLeft.setBias(biasName, biasValue);
            ok = true;
        } else if (channel == "right")
        {
            vsctrlMngRight.setBias(biasName, biasValue);
            ok = true;
        } else if (channel == "")
        {
            vsctrlMngLeft.setBias(biasName, biasValue);
            vsctrlMngRight.setBias(biasName, biasValue);
            ok = true;
        }
        else {
            std::cout << "unrecognised channel" << std::endl;
            ok =false;
        }
    }
        break;
    case COMMAND_VOCAB_PROG:
        rec= true;
    {
        std::string channel = command.get(1).asString();

        // progBias function
        if (channel == "left"){
            vsctrlMngLeft.configureBiases();
            ok = true;
        } else if (channel == "right")
        {
            vsctrlMngRight.configureBiases();
            ok = true;
        }
        else if (channel == "")
        {
            vsctrlMngLeft.configureBiases();
            vsctrlMngRight.configureBiases();
            ok = true;
        } else {
            std::cout << "unrecognised channel" << std::endl;
            ok =false;
        }
    }
        break;
    case COMMAND_VOCAB_PWROFF:
        rec= true;
    {   std::string channel = command.get(1).asString();

        if (channel == "left"){
            vsctrlMngLeft.suspend();
            ok = true;

        } else if (channel == "right") {
            vsctrlMngRight.suspend();
            ok = true;
        } else if (channel == "") { // if channel is not specified power off both
            vsctrlMngRight.suspend();
            vsctrlMngLeft.suspend();
            ok = true;
        } else {
            std::cout << "unrecognised channel" << std::endl;
            ok = false;

        }
    }
        break;

    case COMMAND_VOCAB_PWRON:
        rec= true;
    {   std::string channel = command.get(1).asString();

        if (channel == "left"){
            vsctrlMngLeft.activate();
            ok = true;

        } else if (channel == "right") {
            vsctrlMngRight.activate();
            ok = true;
        } else if (channel == "") { // if channel is not specified power off both
            vsctrlMngRight.activate();
            vsctrlMngLeft.activate();
            ok = true;
        } else {
            std::cout << "unrecognised channel" << std::endl;
            ok = false;

        }
    }
        break;

//    case COMMAND_VOCAB_RST:
//        rec= true;
//    {   std::string channel = command.get(1).asString();

//        if (channel == "left"){
//            vsctrlMngLeft->chipReset();
//            ok = true;

//        } else if (channel == "right") {
//            vsctrlMngRight->chipReset();
//            ok = true;
//        } else if (channel == "") { // if channel is not specified power off both
//            vsctrlMngRight->chipReset();
//            vsctrlMngLeft->chipReset();
//            ok = true;
//        } else {
//            std::cout << "unrecognised channel" << std::endl;
//            ok = false;

//        }
//    }
//        break;


    }

    if (!rec)
        ok = RFModule::respond(command,reply);

    if (!ok) {
        //reply.clear();
        reply.addVocab(COMMAND_VOCAB_FAILED);
    }
    else
        reply.addVocab(COMMAND_VOCAB_OK);

    return ok;

}


