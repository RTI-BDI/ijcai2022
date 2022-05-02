//
// Generated file, do not edit! Created by nedtool 5.6 from src/agentMSG.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include "agentMSG_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i=0; i<n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp


// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');

    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

Register_Class(agentMSG)

agentMSG::agentMSG(const char *name, short kind) : ::omnetpp::cMessage(name,kind)
{
    this->source = -1;
    this->destination = -1;
    this->hopCount = 0;
    this->informative = 0;
    this->ag_id = -1;
    this->ag_task_id = -1;

    this->ag_task_plan_id = -1;                // BDI
    this->ag_sensor_id = -1;                   // BDI
    this->ag_task_compTime = -1;               // BDI
    this->ag_intention_startTime = -1;         // BDI
    this->ag_intention_firstActivation = -1;   // BDI
    this->ag_task_firstActivation = -1;        // BDI

    this->ag_task_demander = -1;
    this->ag_task_server = -1;
    this->ag_task_release_time = -1;
    this->ag_task_ddl = -1;

    this->ag_internal_goal_id = -1;         // BDI
    this->ag_internal_goal_plan_id = -1;    // BDI
    this->ag_internal_goal_original_plan_id = -1; // BDI
    this->ag_internal_goal_name = "";           // BDI
    this->ag_internal_goal_request_time = -1;   // BDI
    this->ag_internal_goal_arrival_time = -1;   // BDI

    this->ag_intention_goal_id = -1; // BDI
    this->ag_intention_plan_id = -1; // BDI
    this->ag_intention_original_plan_id = -1; // BDI
    this->ag_scheduleNewReasoningCycle = false; // BDI
    this->ag_intention_goal_name = ""; // BDI
}

agentMSG::agentMSG(const agentMSG& other) : ::omnetpp::cMessage(other)
{
    copy(other);
}

agentMSG::~agentMSG() { }

agentMSG& agentMSG::operator=(const agentMSG& other)
{
    if (this==&other) return *this;
    ::omnetpp::cMessage::operator=(other);
    copy(other);
    return *this;
}

void agentMSG::copy(const agentMSG& other)
{
    this->source = other.source;
    this->destination = other.destination;
    this->hopCount = other.hopCount;
    this->informative = other.informative;
    this->ag_id = other.ag_id;
    this->ag_task_id = other.ag_task_id;

    this->ag_task_plan_id = other.ag_task_plan_id;                              // BDI
    this->ag_sensor_id = other.ag_sensor_id;                                    // BDI
    this->ag_task_compTime = other.ag_task_compTime;                            // BDI
    this->ag_intention_startTime = other.ag_intention_startTime;                // BDI
    this->ag_intention_firstActivation = other.ag_intention_firstActivation;    // BDI
    this->ag_task_firstActivation = other.ag_task_firstActivation;    // BDI

    this->ag_task_demander = other.ag_task_demander;
    this->ag_task_server = other.ag_task_server;
    this->ag_task_release_time = other.ag_task_release_time;
    this->ag_task_ddl = other.ag_task_ddl;

    this->ag_internal_goal_id = other.ag_internal_goal_id; // BDI
    this->ag_internal_goal_plan_id = other.ag_internal_goal_plan_id; // BDI
    this->ag_internal_goal_original_plan_id = other.ag_internal_goal_original_plan_id; // BDI
    this->ag_internal_goal_name = other.ag_internal_goal_name; // BDI
    this->ag_internal_goal_request_time = other.ag_internal_goal_request_time; // BDI
    this->ag_internal_goal_arrival_time = other.ag_internal_goal_arrival_time; // BDI

    this->ag_intention_goal_id = other.ag_intention_goal_id; // BDI
    this->ag_intention_plan_id = other.ag_intention_plan_id; // BDI
    this->ag_intention_original_plan_id = other.ag_intention_original_plan_id; // BDI
    this->ag_scheduleNewReasoningCycle = other.ag_scheduleNewReasoningCycle; // BDI
    this->ag_intention_goal_name = other.ag_intention_goal_name; // BDI
}

int agentMSG::getSource() const
{
    return this->source;
}

void agentMSG::setSource(int source)
{
    this->source = source;
}

int agentMSG::getDestination() const
{
    return this->destination;
}

void agentMSG::setDestination(int destination)
{
    this->destination = destination;
}

int agentMSG::getHopCount() const
{
    return this->hopCount;
}

void agentMSG::setHopCount(int hopCount)
{
    this->hopCount = hopCount;
}

int agentMSG::getInformative() const
{
    return this->informative;
}

void agentMSG::setInformative(int informative)
{
    this->informative = informative;
}

int agentMSG::getAg_id() const
{
    return this->ag_id;
}

void agentMSG::setAg_id(int ag_id)
{
    this->ag_id = ag_id;
}

int agentMSG::getAg_task_id() const
{
    return this->ag_task_id;
}

void agentMSG::setAg_task_id(int ag_task_id)
{
    this->ag_task_id = ag_task_id;
}

// BDI
int agentMSG::getAg_task_plan_id() const {
    return this->ag_task_plan_id;
}

// BDI
void agentMSG::setAg_task_plan_id(int ag_task_plan_id) {
    this->ag_task_plan_id = ag_task_plan_id;
}

// BDI
int agentMSG::getAg_task_original_plan_id() const {
    return this->ag_task_original_plan_id;
}

// BDI
void agentMSG::setAg_task_original_plan_id(int ag_task_plan_id) {
    this->ag_task_original_plan_id = ag_task_plan_id;
}

// BDI
int agentMSG::getAg_preempted_task_id() const {
    return this->ag_preempted_task_id;
}

// BDI
void agentMSG::setAg_preempted_task_id(int id){
    this->ag_preempted_task_id = id;
}

// BDI
int agentMSG::getAg_preempted_task_plan_id() const{
    return this->ag_preempted_task_plan_id;
}

// BDI
void agentMSG::setAg_preempted_task_plan_id(int id){
    this->ag_preempted_task_plan_id = id;
}

// BDI
int agentMSG::getAg_preempted_task_server_id(){
    return this->ag_preempted_task_server_id;
}

// BDI
void agentMSG::setAg_preempted_task_server_id(int id){
    this->ag_preempted_task_server_id = id;
}

// BDI
int agentMSG::getAg_server_id() {
    return this->ag_server_id;
}

// BDI
void agentMSG::setAg_server_id(int id) {
    this->ag_server_id = id;
}

// BDI
void agentMSG::setAg_task_server(int id){
    this->ag_task_server_id = id;
}

// BDI
int agentMSG::getAg_task_compTime() const {
    return this->ag_task_compTime;
}

// BDI
void agentMSG::setAg_task_compTime(int ag_task_compTime) {
    this->ag_task_compTime = ag_task_compTime;
}

// BDI
double agentMSG::getAg_task_absolute_ddl() const {
    return this->ag_task_absolute_ddl;
}

// BDI
void agentMSG::setAg_task_absolute_ddl(double ag_absolute_ddl) {
    this->ag_task_absolute_ddl = ag_absolute_ddl;
}

// BDI
double agentMSG::getAg_intention_startTime() const {
    return this->ag_intention_startTime;
}

// BDI
void agentMSG::setAg_intention_startTime(double _ag_intention_startTime) {
    this->ag_intention_startTime = _ag_intention_startTime;
}

double agentMSG::getAg_intention_firstActivation() const {
    return this->ag_intention_firstActivation;
}

// BDI
void agentMSG::setAg_intention_firstActivation(double _ag_intention_firstActivation) {
    this->ag_intention_firstActivation = _ag_intention_firstActivation;
}

// BDI
double agentMSG::getAg_task_firstActivation() const {
    return this->ag_task_firstActivation;
}

// BDI
void agentMSG::setAg_task_firstActivation(double time) {
    this->ag_task_firstActivation = time;
}

// BDI
double agentMSG::getAg_task_lastActivation() const {
    return this->ag_task_lastActivation;
}

// BDI
void agentMSG::setAg_task_lastActivation(double time) {
    this->ag_task_lastActivation = time;
}

// JsonFileHandler extra parameters ---------------------------
// BDI
bool agentMSG::getThread_read_file_from_file() const {
    return read_from_file;
}

// BDI
void agentMSG::setThread_read_file_from_file(bool _from_file) {
    read_from_file = _from_file;
}

// Internal goal arrival time ---------------------------
// BDI
int agentMSG::getAg_internal_goal_id() const {
    return ag_internal_goal_id;
}

// BDI
void agentMSG::setAg_internal_goal_id(int id) {
    ag_internal_goal_id = id;
}

// BDI
int agentMSG::getAg_internal_goal_plan_id() const {
    return ag_internal_goal_plan_id;
}

// BDI
void agentMSG::setAg_internal_goal_plan_id(int id) {
    ag_internal_goal_plan_id = id;
}

// BDI
int agentMSG::getAg_internal_goal_original_plan_id() const {
    return ag_internal_goal_original_plan_id;
}

// BDI
void agentMSG::setAg_internal_goal_original_plan_id(int id) {
    ag_internal_goal_original_plan_id = id;
}

// BDI
std::string agentMSG::getAg_internal_goal_name() const {
    return ag_internal_goal_name;
}

// BDI
void agentMSG::setAg_internal_goal_name(std::string name) {
    ag_internal_goal_name = name;
}

// BDI
double agentMSG::getAg_internal_goal_request_time() const {
    return ag_internal_goal_request_time;
}

// BDI
void agentMSG::setAg_internal_goal_request_time(double time) {
    ag_internal_goal_request_time = time;
}

// BDI
double agentMSG::getAg_internal_goal_arrival_time() const {
    return ag_internal_goal_arrival_time;
}

// BDI
void agentMSG::setAg_internal_goal_arrival_time(double time) {
    ag_internal_goal_arrival_time = time;
}
// ------------------------------------------------------

// Intention completed ----------------------------------
// BDI
int agentMSG::getAg_intention_goal_id() const {
    return ag_intention_goal_id;
}

// BDI
void agentMSG::setAg_intention_goal_id(int id) {
    ag_intention_goal_id = id;
}

// BDI
int agentMSG::getAg_intention_plan_id() const {
    return ag_intention_plan_id;
}

// BDI
void agentMSG::setAg_intention_plan_id(int id) {
    ag_intention_plan_id = id;
}

// BDI
int agentMSG::getAg_intention_original_plan_id() const {
    return ag_intention_original_plan_id;
}

// BDI
void agentMSG::setAg_intention_original_plan_id(int id) {
    ag_intention_original_plan_id = id;
}

// BDI
std::string agentMSG::getAg_intention_goal_name() const {
    return ag_intention_goal_name;
}

// BDI
void agentMSG::setAg_intention_goal_name(std::string name) {
    ag_intention_goal_name = name;
}

// BDI
bool agentMSG::getAg_scheduleNewReasoningCycle() const {
    return ag_scheduleNewReasoningCycle;
}

// BDI
void agentMSG::setAg_scheduleNewReasoningCycle(bool val) {
    ag_scheduleNewReasoningCycle = val;
}
// ------------------------------------------------------

// BDI
int agentMSG::getAg_sensor_id() const {
    return this->ag_sensor_id;
}

// BDI
void agentMSG::setAg_sensor_id(int sensor_id) {
    this->ag_sensor_id = sensor_id;
}

// BDI
int agentMSG::getAg_sensor_plan_id() const {
    return this->ag_sensor_plan_id;
}

// BDI
void agentMSG::setAg_sensor_plan_id(int sensor_plan_id) {
    this->ag_sensor_plan_id = sensor_plan_id;
}

// BDI
std::string agentMSG::getAg_sensor_belief_name() const {
    return this->ag_sensor_belief_name;
}

// BDI
void agentMSG::setAg_sensor_belief_name(std::string sensor_belief_name) {
    this->ag_sensor_belief_name = sensor_belief_name;
}

// BDI
bool agentMSG::getAg_apply_reasoning_cycle_again() const {
    return this->apply_reasoning_cycle_again;
}

// BDI
void agentMSG::setAg_apply_reasoning_cycle_again(bool val) {
    this->apply_reasoning_cycle_again = val;
}

int agentMSG::getAg_task_demander() const
{
    return this->ag_task_demander;
}

void agentMSG::setAg_task_demander(int ag_task_demander)
{
    this->ag_task_demander = ag_task_demander;
}

int agentMSG::getAg_task_server() const
{
    return this->ag_task_server;
}

double agentMSG::getAg_task_release_time() const
{
    return this->ag_task_release_time;
}

void agentMSG::setAg_task_release_time(double ag_task_release_time)
{
    this->ag_task_release_time = ag_task_release_time;
}

double agentMSG::getAg_task_ddl() const
{
    return this->ag_task_ddl;
}

void agentMSG::setAg_task_ddl(double ag_task_ddl)
{
    this->ag_task_ddl = ag_task_ddl;
}
