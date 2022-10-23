#include "PM_States.hpp"

/***********************************************************************************************************************
 * Static Member Variable Declarations
 **********************************************************************************************************************/

Telemetry_PIGO_t CommsWithTelemetry::incomingData;
SFOutput_t SensorFusion::sfOutputData;
IMU_Data_t SensorFusion::imudata;
CommandsFromAM CommsWithAttitude::receivedData;


// TAKEOFF STAGE VARIABLES 
LandingTakeoffInput TakeoffStage::input;
LandingTakeoffOutput TakeoffStage::output;
WaypointData * TakeoffStage::currentLocation;    
WaypointData * TakeoffStage::targetWaypoint;
CommandsForAM_t TakeoffStage::takeoffDataForAM; 

// LANDING STAGE VARIABLES 
LandingTakeoffInput LandingStage::input;
LandingTakeoffOutput LandingStage::output;
WaypointData * LandingStage::currentLocation;
WaypointData * LandingStage::targetWaypoint;
CommandsForAM_t LandingStage::landingDataForAM;


constexpr int LANDING_TIME_THRESHOLD {5};


/***********************************************************************************************************************
 * Code
 **********************************************************************************************************************/

void CommsWithAttitude::execute(pathManager* pathMgr)
{
    
    // Getting data from AM 
    bool newDataAvailable = GetFromAMToPM(&receivedData);
    
  

    /*_WaypointManager_Data_Out * waypointOutput {}; 
    
    //deciding which stage we get output data from 
    switch(pathMgr->flight_stage){
        case LANDING:
            waypointOutput = LandingStage::GetOutputData();
            break;
        case CRUISING:
            waypointOutput = cruisingState::GetOutputData();
            break;
        case PREFLIGHT:
            waypointOutput = PreflightStage::GetOutputData();
            break;
        case TAKEOFF:
           waypointOutput = TakeoffStage::GetOutputData();
            break;
        default:
            waypointOutput = cruisingState::GetOutputData();
    }
    */
    
    CommandsForAM toSend {};
    switch(pathMgr->flight_stage){
            case LANDING:
                toSend = LandingStage::getLandingDataForAM();
                break;
            case CRUISING:
               // waypointOutput = cruisingState::GetOutputData();
                break;
            case PREFLIGHT:
               // waypointOutput = PreflightStage::GetOutputData();
                break;
            case TAKEOFF:
                toSend = TakeoffStage::getTakeoffDataForAM(); 
                break;
            default:
        }
        
    SendFromPMToAM(&toSend); // Sends commands to attitude manager
    

     if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(CommsWithTelemetry::getInstance());
    }
}

pathManagerState& CommsWithAttitude::getInstance()
{
    static CommsWithAttitude singleton;
    return singleton;
}

void CommsWithTelemetry::execute(pathManager* pathMgr)
{
    GetTelemetryCommands(&_incomingData);

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(SensorFusion::getInstance());
    }
}

pathManagerState& CommsWithTelemetry::getInstance()
{
    static CommsWithTelemetry singleton;
    return singleton;
}

void SensorFusion::execute(pathManager* pathMgr)
{
    SFError_t error = SF_GetResult(&_sfOutputData); // Gets current Sensor fusion output struct
    _imudata = SF_GetRawIMU();

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(FlightModeSelector::getInstance());
    }
}

pathManagerState& SensorFusion::getInstance()
{
    static SensorFusion singleton;
    return singleton;
}

#ifdef UNIT_TESTING

#include "cruisingMode.hpp"

static Telemetry_Waypoint_Data_t createTelemetryWaypoint(long double lon, long double lat, int alt, float turnRadius, int type) {
    Telemetry_Waypoint_Data_t waypoint {};

    waypoint.longitude = lon;
    waypoint.latitude = lat;
    waypoint.altitude = alt;
    waypoint.turnRadius = turnRadius;
    waypoint.waypointType = type;

    return waypoint;
}

#endif
void FlightModeSelector::execute(pathManager* pathMgr){
    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }

    // May need to go to a preflight stage later
    /*if(CommsWithTelemetry::GetTelemetryIncomingData()->takeoffCommand && SensorFusion::GetSFOutput()->altitude < ON_GROUND_ALT)
    {
        pathMgr->flight_stage = PREFLIGHT;

    }*/

    // Prev code had a preflight stage for motor spin up...may be used later. 
    //if (pathMgr->flight_stage == PREFLIGHT && PreflightStage::getPreFlightCompleteStatus){


    // Checking if we are going from disarmed to armed to start takeoff stage. 
    if (CommsWithAttitude::GetCommFromAttitude()->armed && pathMgr->flight_stage == DISARMED){
        pathMgr->flight_stage = TAKEOFF; 

    }

    // Check if altitude target has been reached for takeoff to start cruising stage. 
    if(SensorFusion::GetSFOutput()->altitude > LandingTakeoffManager::getTakeoffAltitudeTarget && pathMgr->flight_stage == TAKEOFF)
    {
        pathMgr->flight_stage = CRUISING;

    }

     if(!CommsWithTelemetry::GetTelemetryIncomingData()->takeoffCommand)
    {
        pathMgr->flight_stage = LANDING;
    
    }

    // Assuming SF can give elapsed time, check if below landing target for a certain amount of time to start landed stage. 
    if (SensorFusion::GetSFOutput()->altitude < LandingTakeoffManager::getLandingAltitudeTarget && SensorFusion::GetSFOutput()->deltaTime > LANDING_TIME_THRESHOLD){
        pathMgr->flight_stage = LANDED; 
    }

    //when flight time passes threshold -> Landed 
 
    switch(pathMgr->flight_stage){
        case LANDING:
            pathMgr->setState(LandingStage::getInstance());
            break;
        case CRUISING:
            //no control details need to be reset for cruising state
            pathMgr->setState(cruisingState::getInstance());
            break;
        case PREFLIGHT:
            pathMgr->setState(PreflightStage::getInstance());
            break;
        case TAKEOFF:
            pathMgr->setState(TakeoffStage::getInstance());
            break;
        default:
            // Any other stage (Disarmed, Landed) should go back to comms with AM
            pathMgr->setState(CommsWithAttitude::getInstance()); 
    }
}

pathManagerState& FlightModeSelector::getInstance()
{
    static FlightModeSelector singleton; 
    return singleton; 
}

void CruisingStage::execute(pathManager* pathMgr)
{
    /*
    // telementryData is where we want to go, target waypoint
    fijo * telemetryData = CommsWithTelemetry::GetTelemetryIncomingData(); // Get struct from telemetry state with all of the commands and values.
    SFOutput_t * sensFusionOutput = SensorFusion::GetSFOutput(); // Get sensor fusion data

    // Set waypoint manager input struct 
    // input data is the current position, should be middle waypoint
    _inputdata.track = sensFusionOutput->track; // Gets track
    _inputdata.longitude = sensFusionOutput->longitude; // setting current waypoint longitude to SF longitude 
    _inputdata.latitude = sensFusionOutput->latitude;
    _inputdata.altitude = sensFusionOutput->altitude;

    // Call module functions - These will be changed later 

    //initializing current and target waypoint, setting the relationship of their pointers 
    ModifyFlightPathErrorCode editError = editFlightPath(cruisingStateManager, telemetryData, this->_inputdata); // Edit flight path if applicable
    
    
    GetNextDirectionsErrorCode pathError = pathFollow(cruisingStateManager, telemetryData, _inputdata, &_outputdata); // Get next direction or modify flight behaviour pattern
    setReturnValues(&_returnToGround, cruisingStateManager, editError, pathError); // Set error codes

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        // pathMgr->setState(coordinateTurnElevation::getInstance());
         pathMgr->setState(CommsWithAttitude::getInstance()); 
    }
    */
}

pathManagerState& CruisingStage::getInstance()
{
    static CruisingStage singleton;
    return singleton;
}

void LandingStage::execute(pathManager* pathMgr)
{
    pathMgr->flight_stage = LANDING;

    //load in sensor fusion data and telemtry data into input structure
    input.telemetryData = CommsWithTelemetry::GetTelemetryIncomingData();
    input.sensorOutput = SensorFusion::GetSFOutput();

   // Creating AM struct to send takeoff data using current SF data.
    landingDataForAM = LandingTakeoffManager::createLandingWaypoint(input.sensorOutput);    

   /* waypointInput.latitude = input.sensorOutput->latitude;
    waypointInput.longitude = input.sensorOutput->longitude;
    waypointInput.altitude = input.sensorOutput->altitude;
    waypointInput.track = input.sensorOutput->track;*/

    if(LandingStage::waypointStatus != WAYPOINT_SUCCESS)
    {
        pathMgr->isError = true;
    }

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(CommsWithAttitude::getInstance());
    }
}

pathManagerState& LandingStage::getInstance()
{
    static LandingStage singleton;
    return singleton;
}


/****************************************************************************************************
TAKEOFF STATE FUNCTIONS
****************************************************************************************************/
//create a count to 1000 -> then proceed to takeoff. 
void PreflightStage::execute(pathManager* pathMgr)
{
    //load in sensor fusion data and telemtry data into input structure
    input.telemetryData = CommsWithTelemetry::GetTelemetryIncomingData();
    input.sensorOutput = SensorFusion::GetSFOutput();

    
    // This is the part where we ~wait~ before going to takeoff...not sure if it will work 

    if (cycleCount < 1000){
        cycleCount++; 
        preFlightComplete = false;
    } else { 
        preFlightComplete = true; 
        cycleCount = 0; 
    }


    if(PreflightStage::waypointStatus != WAYPOINT_SUCCESS)
    {
        pathMgr->isError = true;
    }

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(CommsWithAttitude::getInstance());
    }
}

pathManagerState& PreflightStage::getInstance()
{
    static PreflightStage singleton;
    return singleton;
}

void TakeoffStage::execute(pathManager* pathMgr)
{
    //load in sensor fusion data and telemtry data into input structure
    input.telemetryData = CommsWithTelemetry::GetTelemetryIncomingData();
    input.sensorOutput = SensorFusion::GetSFOutput();

       /** waypointInput.latitude = input.sensorOutput->latitude;
        waypointInput.longitude = input.sensorOutput->longitude;
        waypointInput.altitude = input.sensorOutput->altitude;
        //waypointInput.track = input.sensorOutput->track;*/ 

    // Creating AM struct to send takeoff data using current SF data.
    takeoffDataForAM = LandingTakeoffManager::createTakeoffWaypoint(input.sensorOutput);    
        

    if(waypointStatus != WAYPOINT_SUCCESS)
    {
        pathMgr->isError = true;
    }

    if(pathMgr->isError)
    {
        pathMgr->setState(fatalFailureMode::getInstance());
    }
    else
    {
        pathMgr->setState(CommsWithAttitude::getInstance());
    }
}

pathManagerState& TakeoffStage::getInstance()
{
    static TakeoffStage singleton;
    return singleton;
}

void fatalFailureMode::execute(pathManager* pathMgr)
{
    pathMgr->setState(fatalFailureMode::getInstance());
}

pathManagerState& fatalFailureMode::getInstance()
{
    static fatalFailureMode singleton;
    return singleton;
}


