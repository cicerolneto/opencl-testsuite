package com.lepisto.mikael.ocltester;

import android.annotation.SuppressLint;
import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

/**
 * To setup android emulator to redirect ports to be visible in host do
 */
public class OclCallService extends Service {

    // Message types for communication with sourceActivity
    public static final int MSG_TYPE_COMMAND = 1;
    public static final int MSG_TYPE_RESULT = 2;
    public static final int MSG_TYPE_ERROR = 3;
    public static final int MSG_TYPE_SERVICE_DISCONNECTED = 4;

    public static final String MSG_KEY_COMMAND = "command";
    public static final String MSG_KEY_RESPONSE = "response";
    public static final String MSG_COMMAND_COMPILE = "compile";
    public static final String MSG_COMMAND_INFO = "info";

    /**
     * Inter Process Communication...
     */
    @SuppressLint("HandlerLeak")
	class IncomingHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {

            if (msg.what != MSG_TYPE_COMMAND) {
                super.handleMessage(msg);
            }

            Bundle input = msg.getData();
            String command = input.getString(MSG_KEY_COMMAND);

            Bundle replyData = new Bundle();
            int replyType = MSG_TYPE_RESULT;

            if (msg.what == MSG_TYPE_COMMAND) {
                if (command.equals(MSG_COMMAND_INFO)) {
                    String devInfo = oclTester.getDeviceInfo();
                    replyData.putString(MSG_KEY_RESPONSE, devInfo);
                } else if (command.equals(MSG_COMMAND_COMPILE)) {
                    // TODO: get code from input.getString("code");...
                    // if crash, won't be finished..
                    String compileResults = oclTester.compileWithDevice("1050148873", "kernel void zero_one_or_other(void) {local uint local_1[1];local uint local_2[1];*(local_1 > local_2 ? local_1 : local_2) = 0;}");
                    replyData.putString(MSG_KEY_RESPONSE, compileResults);
                } else {
                    replyType = MSG_TYPE_ERROR;
                    replyData.putString(MSG_KEY_RESPONSE, "Invalid command: "+ command);
                }
            } else {
                replyType = MSG_TYPE_ERROR;
                replyData.putString(MSG_KEY_RESPONSE, "Invalid message type: "+ msg.what);                
            }

            // Reply
            Message reply = Message.obtain(null, replyType);
            reply.setData(replyData);            
            try {
                msg.replyTo.send(reply);
            } catch (RemoteException e) {
                // TODO: Maybe we should disconnect in this case so that
                // client know that something went wrong...
                e.printStackTrace();
            }
        }
    }

    final Messenger mMessenger = new Messenger(new IncomingHandler());
    final OclTester oclTester = new OclTester();

    /**
     * When binding to the service, we return an interface to our messenger
     * for sending messages to the service.
     */
    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }
}

