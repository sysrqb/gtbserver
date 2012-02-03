// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: communication.proto

#ifndef PROTOBUF_communication_2eproto__INCLUDED
#define PROTOBUF_communication_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2004000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2004001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_message_reflection.h>
#include "patron.pb.h"
// @@protoc_insertion_point(includes)

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_communication_2eproto();
void protobuf_AssignDesc_communication_2eproto();
void protobuf_ShutdownFile_communication_2eproto();

class Request;
class Response;

// ===================================================================

class Request : public ::google::protobuf::Message {
 public:
  Request();
  virtual ~Request();
  
  Request(const Request& from);
  
  inline Request& operator=(const Request& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const Request& default_instance();
  
  void Swap(Request* other);
  
  // implements Message ----------------------------------------------
  
  Request* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Request& from);
  void MergeFrom(const Request& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // required int32 nReqId = 1;
  inline bool has_nreqid() const;
  inline void clear_nreqid();
  static const int kNReqIdFieldNumber = 1;
  inline ::google::protobuf::int32 nreqid() const;
  inline void set_nreqid(::google::protobuf::int32 value);
  
  // required string sReqType = 2;
  inline bool has_sreqtype() const;
  inline void clear_sreqtype();
  static const int kSReqTypeFieldNumber = 2;
  inline const ::std::string& sreqtype() const;
  inline void set_sreqtype(const ::std::string& value);
  inline void set_sreqtype(const char* value);
  inline void set_sreqtype(const char* value, size_t size);
  inline ::std::string* mutable_sreqtype();
  inline ::std::string* release_sreqtype();
  
  // optional int32 nCarId = 3;
  inline bool has_ncarid() const;
  inline void clear_ncarid();
  static const int kNCarIdFieldNumber = 3;
  inline ::google::protobuf::int32 ncarid() const;
  inline void set_ncarid(::google::protobuf::int32 value);
  
  // repeated string sParams = 4;
  inline int sparams_size() const;
  inline void clear_sparams();
  static const int kSParamsFieldNumber = 4;
  inline const ::std::string& sparams(int index) const;
  inline ::std::string* mutable_sparams(int index);
  inline void set_sparams(int index, const ::std::string& value);
  inline void set_sparams(int index, const char* value);
  inline void set_sparams(int index, const char* value, size_t size);
  inline ::std::string* add_sparams();
  inline void add_sparams(const ::std::string& value);
  inline void add_sparams(const char* value);
  inline void add_sparams(const char* value, size_t size);
  inline const ::google::protobuf::RepeatedPtrField< ::std::string>& sparams() const;
  inline ::google::protobuf::RepeatedPtrField< ::std::string>* mutable_sparams();
  
  // repeated int32 nParams = 5 [packed = true];
  inline int nparams_size() const;
  inline void clear_nparams();
  static const int kNParamsFieldNumber = 5;
  inline ::google::protobuf::int32 nparams(int index) const;
  inline void set_nparams(int index, ::google::protobuf::int32 value);
  inline void add_nparams(::google::protobuf::int32 value);
  inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      nparams() const;
  inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_nparams();
  
  // optional .PatronList plPatronList = 6;
  inline bool has_plpatronlist() const;
  inline void clear_plpatronlist();
  static const int kPlPatronListFieldNumber = 6;
  inline const ::PatronList& plpatronlist() const;
  inline ::PatronList* mutable_plpatronlist();
  inline ::PatronList* release_plpatronlist();
  
  // @@protoc_insertion_point(class_scope:Request)
 private:
  inline void set_has_nreqid();
  inline void clear_has_nreqid();
  inline void set_has_sreqtype();
  inline void clear_has_sreqtype();
  inline void set_has_ncarid();
  inline void clear_has_ncarid();
  inline void set_has_plpatronlist();
  inline void clear_has_plpatronlist();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* sreqtype_;
  ::google::protobuf::int32 nreqid_;
  ::google::protobuf::int32 ncarid_;
  ::google::protobuf::RepeatedPtrField< ::std::string> sparams_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > nparams_;
  mutable int _nparams_cached_byte_size_;
  ::PatronList* plpatronlist_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(6 + 31) / 32];
  
  friend void  protobuf_AddDesc_communication_2eproto();
  friend void protobuf_AssignDesc_communication_2eproto();
  friend void protobuf_ShutdownFile_communication_2eproto();
  
  void InitAsDefaultInstance();
  static Request* default_instance_;
};
// -------------------------------------------------------------------

class Response : public ::google::protobuf::Message {
 public:
  Response();
  virtual ~Response();
  
  Response(const Response& from);
  
  inline Response& operator=(const Response& from) {
    CopyFrom(from);
    return *this;
  }
  
  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }
  
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }
  
  static const ::google::protobuf::Descriptor* descriptor();
  static const Response& default_instance();
  
  void Swap(Response* other);
  
  // implements Message ----------------------------------------------
  
  Response* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Response& from);
  void MergeFrom(const Response& from);
  void Clear();
  bool IsInitialized() const;
  
  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  
  ::google::protobuf::Metadata GetMetadata() const;
  
  // nested types ----------------------------------------------------
  
  // accessors -------------------------------------------------------
  
  // required int32 nRespId = 1;
  inline bool has_nrespid() const;
  inline void clear_nrespid();
  static const int kNRespIdFieldNumber = 1;
  inline ::google::protobuf::int32 nrespid() const;
  inline void set_nrespid(::google::protobuf::int32 value);
  
  // required string sResValue = 2;
  inline bool has_sresvalue() const;
  inline void clear_sresvalue();
  static const int kSResValueFieldNumber = 2;
  inline const ::std::string& sresvalue() const;
  inline void set_sresvalue(const ::std::string& value);
  inline void set_sresvalue(const char* value);
  inline void set_sresvalue(const char* value, size_t size);
  inline ::std::string* mutable_sresvalue();
  inline ::std::string* release_sresvalue();
  
  // repeated string sResAdd = 3;
  inline int sresadd_size() const;
  inline void clear_sresadd();
  static const int kSResAddFieldNumber = 3;
  inline const ::std::string& sresadd(int index) const;
  inline ::std::string* mutable_sresadd(int index);
  inline void set_sresadd(int index, const ::std::string& value);
  inline void set_sresadd(int index, const char* value);
  inline void set_sresadd(int index, const char* value, size_t size);
  inline ::std::string* add_sresadd();
  inline void add_sresadd(const ::std::string& value);
  inline void add_sresadd(const char* value);
  inline void add_sresadd(const char* value, size_t size);
  inline const ::google::protobuf::RepeatedPtrField< ::std::string>& sresadd() const;
  inline ::google::protobuf::RepeatedPtrField< ::std::string>* mutable_sresadd();
  
  // repeated int32 nResAdd = 4 [packed = true];
  inline int nresadd_size() const;
  inline void clear_nresadd();
  static const int kNResAddFieldNumber = 4;
  inline ::google::protobuf::int32 nresadd(int index) const;
  inline void set_nresadd(int index, ::google::protobuf::int32 value);
  inline void add_nresadd(::google::protobuf::int32 value);
  inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      nresadd() const;
  inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_nresadd();
  
  // optional .PatronList plPatronList = 5;
  inline bool has_plpatronlist() const;
  inline void clear_plpatronlist();
  static const int kPlPatronListFieldNumber = 5;
  inline const ::PatronList& plpatronlist() const;
  inline ::PatronList* mutable_plpatronlist();
  inline ::PatronList* release_plpatronlist();
  
  // @@protoc_insertion_point(class_scope:Response)
 private:
  inline void set_has_nrespid();
  inline void clear_has_nrespid();
  inline void set_has_sresvalue();
  inline void clear_has_sresvalue();
  inline void set_has_plpatronlist();
  inline void clear_has_plpatronlist();
  
  ::google::protobuf::UnknownFieldSet _unknown_fields_;
  
  ::std::string* sresvalue_;
  ::google::protobuf::RepeatedPtrField< ::std::string> sresadd_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > nresadd_;
  mutable int _nresadd_cached_byte_size_;
  ::PatronList* plpatronlist_;
  ::google::protobuf::int32 nrespid_;
  
  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(5 + 31) / 32];
  
  friend void  protobuf_AddDesc_communication_2eproto();
  friend void protobuf_AssignDesc_communication_2eproto();
  friend void protobuf_ShutdownFile_communication_2eproto();
  
  void InitAsDefaultInstance();
  static Response* default_instance_;
};
// ===================================================================


// ===================================================================

// Request

// required int32 nReqId = 1;
inline bool Request::has_nreqid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Request::set_has_nreqid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Request::clear_has_nreqid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Request::clear_nreqid() {
  nreqid_ = 0;
  clear_has_nreqid();
}
inline ::google::protobuf::int32 Request::nreqid() const {
  return nreqid_;
}
inline void Request::set_nreqid(::google::protobuf::int32 value) {
  set_has_nreqid();
  nreqid_ = value;
}

// required string sReqType = 2;
inline bool Request::has_sreqtype() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Request::set_has_sreqtype() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Request::clear_has_sreqtype() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Request::clear_sreqtype() {
  if (sreqtype_ != &::google::protobuf::internal::kEmptyString) {
    sreqtype_->clear();
  }
  clear_has_sreqtype();
}
inline const ::std::string& Request::sreqtype() const {
  return *sreqtype_;
}
inline void Request::set_sreqtype(const ::std::string& value) {
  set_has_sreqtype();
  if (sreqtype_ == &::google::protobuf::internal::kEmptyString) {
    sreqtype_ = new ::std::string;
  }
  sreqtype_->assign(value);
}
inline void Request::set_sreqtype(const char* value) {
  set_has_sreqtype();
  if (sreqtype_ == &::google::protobuf::internal::kEmptyString) {
    sreqtype_ = new ::std::string;
  }
  sreqtype_->assign(value);
}
inline void Request::set_sreqtype(const char* value, size_t size) {
  set_has_sreqtype();
  if (sreqtype_ == &::google::protobuf::internal::kEmptyString) {
    sreqtype_ = new ::std::string;
  }
  sreqtype_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Request::mutable_sreqtype() {
  set_has_sreqtype();
  if (sreqtype_ == &::google::protobuf::internal::kEmptyString) {
    sreqtype_ = new ::std::string;
  }
  return sreqtype_;
}
inline ::std::string* Request::release_sreqtype() {
  clear_has_sreqtype();
  if (sreqtype_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = sreqtype_;
    sreqtype_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// optional int32 nCarId = 3;
inline bool Request::has_ncarid() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Request::set_has_ncarid() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Request::clear_has_ncarid() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Request::clear_ncarid() {
  ncarid_ = 0;
  clear_has_ncarid();
}
inline ::google::protobuf::int32 Request::ncarid() const {
  return ncarid_;
}
inline void Request::set_ncarid(::google::protobuf::int32 value) {
  set_has_ncarid();
  ncarid_ = value;
}

// repeated string sParams = 4;
inline int Request::sparams_size() const {
  return sparams_.size();
}
inline void Request::clear_sparams() {
  sparams_.Clear();
}
inline const ::std::string& Request::sparams(int index) const {
  return sparams_.Get(index);
}
inline ::std::string* Request::mutable_sparams(int index) {
  return sparams_.Mutable(index);
}
inline void Request::set_sparams(int index, const ::std::string& value) {
  sparams_.Mutable(index)->assign(value);
}
inline void Request::set_sparams(int index, const char* value) {
  sparams_.Mutable(index)->assign(value);
}
inline void Request::set_sparams(int index, const char* value, size_t size) {
  sparams_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Request::add_sparams() {
  return sparams_.Add();
}
inline void Request::add_sparams(const ::std::string& value) {
  sparams_.Add()->assign(value);
}
inline void Request::add_sparams(const char* value) {
  sparams_.Add()->assign(value);
}
inline void Request::add_sparams(const char* value, size_t size) {
  sparams_.Add()->assign(reinterpret_cast<const char*>(value), size);
}
inline const ::google::protobuf::RepeatedPtrField< ::std::string>&
Request::sparams() const {
  return sparams_;
}
inline ::google::protobuf::RepeatedPtrField< ::std::string>*
Request::mutable_sparams() {
  return &sparams_;
}

// repeated int32 nParams = 5 [packed = true];
inline int Request::nparams_size() const {
  return nparams_.size();
}
inline void Request::clear_nparams() {
  nparams_.Clear();
}
inline ::google::protobuf::int32 Request::nparams(int index) const {
  return nparams_.Get(index);
}
inline void Request::set_nparams(int index, ::google::protobuf::int32 value) {
  nparams_.Set(index, value);
}
inline void Request::add_nparams(::google::protobuf::int32 value) {
  nparams_.Add(value);
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
Request::nparams() const {
  return nparams_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
Request::mutable_nparams() {
  return &nparams_;
}

// optional .PatronList plPatronList = 6;
inline bool Request::has_plpatronlist() const {
  return (_has_bits_[0] & 0x00000020u) != 0;
}
inline void Request::set_has_plpatronlist() {
  _has_bits_[0] |= 0x00000020u;
}
inline void Request::clear_has_plpatronlist() {
  _has_bits_[0] &= ~0x00000020u;
}
inline void Request::clear_plpatronlist() {
  if (plpatronlist_ != NULL) plpatronlist_->::PatronList::Clear();
  clear_has_plpatronlist();
}
inline const ::PatronList& Request::plpatronlist() const {
  return plpatronlist_ != NULL ? *plpatronlist_ : *default_instance_->plpatronlist_;
}
inline ::PatronList* Request::mutable_plpatronlist() {
  set_has_plpatronlist();
  if (plpatronlist_ == NULL) plpatronlist_ = new ::PatronList;
  return plpatronlist_;
}
inline ::PatronList* Request::release_plpatronlist() {
  clear_has_plpatronlist();
  ::PatronList* temp = plpatronlist_;
  plpatronlist_ = NULL;
  return temp;
}

// -------------------------------------------------------------------

// Response

// required int32 nRespId = 1;
inline bool Response::has_nrespid() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Response::set_has_nrespid() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Response::clear_has_nrespid() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Response::clear_nrespid() {
  nrespid_ = 0;
  clear_has_nrespid();
}
inline ::google::protobuf::int32 Response::nrespid() const {
  return nrespid_;
}
inline void Response::set_nrespid(::google::protobuf::int32 value) {
  set_has_nrespid();
  nrespid_ = value;
}

// required string sResValue = 2;
inline bool Response::has_sresvalue() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Response::set_has_sresvalue() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Response::clear_has_sresvalue() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Response::clear_sresvalue() {
  if (sresvalue_ != &::google::protobuf::internal::kEmptyString) {
    sresvalue_->clear();
  }
  clear_has_sresvalue();
}
inline const ::std::string& Response::sresvalue() const {
  return *sresvalue_;
}
inline void Response::set_sresvalue(const ::std::string& value) {
  set_has_sresvalue();
  if (sresvalue_ == &::google::protobuf::internal::kEmptyString) {
    sresvalue_ = new ::std::string;
  }
  sresvalue_->assign(value);
}
inline void Response::set_sresvalue(const char* value) {
  set_has_sresvalue();
  if (sresvalue_ == &::google::protobuf::internal::kEmptyString) {
    sresvalue_ = new ::std::string;
  }
  sresvalue_->assign(value);
}
inline void Response::set_sresvalue(const char* value, size_t size) {
  set_has_sresvalue();
  if (sresvalue_ == &::google::protobuf::internal::kEmptyString) {
    sresvalue_ = new ::std::string;
  }
  sresvalue_->assign(reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Response::mutable_sresvalue() {
  set_has_sresvalue();
  if (sresvalue_ == &::google::protobuf::internal::kEmptyString) {
    sresvalue_ = new ::std::string;
  }
  return sresvalue_;
}
inline ::std::string* Response::release_sresvalue() {
  clear_has_sresvalue();
  if (sresvalue_ == &::google::protobuf::internal::kEmptyString) {
    return NULL;
  } else {
    ::std::string* temp = sresvalue_;
    sresvalue_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
    return temp;
  }
}

// repeated string sResAdd = 3;
inline int Response::sresadd_size() const {
  return sresadd_.size();
}
inline void Response::clear_sresadd() {
  sresadd_.Clear();
}
inline const ::std::string& Response::sresadd(int index) const {
  return sresadd_.Get(index);
}
inline ::std::string* Response::mutable_sresadd(int index) {
  return sresadd_.Mutable(index);
}
inline void Response::set_sresadd(int index, const ::std::string& value) {
  sresadd_.Mutable(index)->assign(value);
}
inline void Response::set_sresadd(int index, const char* value) {
  sresadd_.Mutable(index)->assign(value);
}
inline void Response::set_sresadd(int index, const char* value, size_t size) {
  sresadd_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
}
inline ::std::string* Response::add_sresadd() {
  return sresadd_.Add();
}
inline void Response::add_sresadd(const ::std::string& value) {
  sresadd_.Add()->assign(value);
}
inline void Response::add_sresadd(const char* value) {
  sresadd_.Add()->assign(value);
}
inline void Response::add_sresadd(const char* value, size_t size) {
  sresadd_.Add()->assign(reinterpret_cast<const char*>(value), size);
}
inline const ::google::protobuf::RepeatedPtrField< ::std::string>&
Response::sresadd() const {
  return sresadd_;
}
inline ::google::protobuf::RepeatedPtrField< ::std::string>*
Response::mutable_sresadd() {
  return &sresadd_;
}

// repeated int32 nResAdd = 4 [packed = true];
inline int Response::nresadd_size() const {
  return nresadd_.size();
}
inline void Response::clear_nresadd() {
  nresadd_.Clear();
}
inline ::google::protobuf::int32 Response::nresadd(int index) const {
  return nresadd_.Get(index);
}
inline void Response::set_nresadd(int index, ::google::protobuf::int32 value) {
  nresadd_.Set(index, value);
}
inline void Response::add_nresadd(::google::protobuf::int32 value) {
  nresadd_.Add(value);
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
Response::nresadd() const {
  return nresadd_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
Response::mutable_nresadd() {
  return &nresadd_;
}

// optional .PatronList plPatronList = 5;
inline bool Response::has_plpatronlist() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void Response::set_has_plpatronlist() {
  _has_bits_[0] |= 0x00000010u;
}
inline void Response::clear_has_plpatronlist() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void Response::clear_plpatronlist() {
  if (plpatronlist_ != NULL) plpatronlist_->::PatronList::Clear();
  clear_has_plpatronlist();
}
inline const ::PatronList& Response::plpatronlist() const {
  return plpatronlist_ != NULL ? *plpatronlist_ : *default_instance_->plpatronlist_;
}
inline ::PatronList* Response::mutable_plpatronlist() {
  set_has_plpatronlist();
  if (plpatronlist_ == NULL) plpatronlist_ = new ::PatronList;
  return plpatronlist_;
}
inline ::PatronList* Response::release_plpatronlist() {
  clear_has_plpatronlist();
  ::PatronList* temp = plpatronlist_;
  plpatronlist_ = NULL;
  return temp;
}


// @@protoc_insertion_point(namespace_scope)

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_communication_2eproto__INCLUDED