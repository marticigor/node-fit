#include <string>
#include <string.h>
#include <node.h>
#include <v8.h>

#include <fstream>
#include <iostream>

#include "fitparser.h"
#include "listener.h"
#include "../libfit/fit_decode.hpp"
#include "../libfit/fit_mesg_broadcaster.hpp"

using namespace std;
using namespace v8;
using namespace node;

FitParser::FitParser() {};
FitParser::~FitParser() {};

void FitParser::Init(Handle<Object> target) {
   // Prepare constructor template
   Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
   tpl->SetClassName(String::NewSymbol("FitParser"));
   tpl->InstanceTemplate()->SetInternalFieldCount(1);
   // Prototype
   tpl->PrototypeTemplate()->Set(String::NewSymbol("decode"),
      FunctionTemplate::New(Decode)->GetFunction());

   Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
   target->Set(String::NewSymbol("FitParser"), constructor);
}

Handle<Value> FitParser::New(const Arguments& args) {
   HandleScope scope;

   assert(args.IsConstructCall());
   FitParser* self = new FitParser();
   self->Wrap(args.This());

   printf("New FitParser!\n");
   return args.This();
}

Handle<Value> FitParser::Decode(const Arguments& args) {
   HandleScope scope;
   fit::Decode decode;
   fit::MesgBroadcaster mesgBroadcaster;
   Listener* listener = new Listener(args);
   std::fstream file;

   if (args.Length() < 1) {
      ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
      return scope.Close(Undefined());
   }

   if (!args[0]->IsString()) {
      ThrowException(Exception::TypeError(String::New("Wrong arguments")));
      return scope.Close(Undefined());
   }

   String::Utf8Value fileName(args[0]->ToString());
   
   printf("Decoding %s!\n", *fileName);
   file.open(strdup(*fileName), std::ios::in | std::ios::binary);

   if (!file.is_open())
   {
      ThrowException(Exception::TypeError(String::New("Error opening file.")));
      return scope.Close(Undefined());
   }

   if (!decode.CheckIntegrity(file))
   {
      ThrowException(Exception::TypeError(String::New("FIT file integrity failed.")));
      return scope.Close(Undefined());
   }

   mesgBroadcaster.AddListener((fit::MesgListener &)listener);

   try
   {
      printf("Running %s!\n", *fileName);
      mesgBroadcaster.Run(file);
   }
   catch (const fit::RuntimeException& e)
   {
      ThrowException(Exception::TypeError(String::New("Exception while decoding file.")));
      return scope.Close(Undefined());
   }

   printf("End!\n");
   Handle<Value> argv[2] = {
    String::New("end"), // event name
    args[0]->ToString()  // argument
   };

   MakeCallback(args.This(), "emit", 2, argv);

   return Undefined();
}