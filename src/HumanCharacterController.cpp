#include "stdafx.h"
#include "HumanCharacterController.h"
#include "Pedestrian.h"
#include "PhysicsManager.h"
#include "PhysicsComponents.h"
#include "PhysicsDefs.h"
#include "Vehicle.h"

//////////////////////////////////////////////////////////////////////////

struct ActionDefaulMapping
{
public:
    ActionDefaulMapping(ePedestrianAction action, eKeycode keycode, eGamepadButton gpButton = eGamepadButton_null)
        : mAction(action)
        , mKeycode(keycode)
        , mGpButton(gpButton)
    {
    }
public:
    ePedestrianAction mAction;
    eKeycode mKeycode;
    eGamepadButton mGpButton;
};

static const ActionDefaulMapping ActionsInCar[] = 
{
    {ePedestrianAction_LeaveCar,            eKeycode_ENTER},
    {ePedestrianAction_HandBrake,           eKeycode_SPACE},
    {ePedestrianAction_Accelerate,          eKeycode_UP},
    {ePedestrianAction_Reverse,             eKeycode_DOWN},
    {ePedestrianAction_SteerLeft,           eKeycode_LEFT},
    {ePedestrianAction_SteerRight,          eKeycode_RIGHT},
    {ePedestrianAction_Horn,                eKeycode_TAB},
};

static const ActionDefaulMapping ActionsOnFoot[] =
{
    {ePedestrianAction_TurnLeft,            eKeycode_LEFT},
    {ePedestrianAction_TurnRight,           eKeycode_RIGHT},
    {ePedestrianAction_Jump,                eKeycode_SPACE},
    {ePedestrianAction_WalkBackward,        eKeycode_DOWN},
    {ePedestrianAction_Run,                 eKeycode_UP},
    {ePedestrianAction_Shoot,               eKeycode_LEFT_CTRL},
    {ePedestrianAction_NextWeapon,          eKeycode_X},
    {ePedestrianAction_PrevWeapon,          eKeycode_Z},
    {ePedestrianAction_EnterCar,            eKeycode_ENTER},
    {ePedestrianAction_EnterCarAsPassenger, eKeycode_F},
};

InputActionsMapping::InputActionsMapping()
{
    SetNull();
}

void InputActionsMapping::SetNull()
{
    mControllerType = eInputControllerType_None;

    ::memset(mKeycodes, 0, sizeof(mKeycodes));
    ::memset(mGpButtons, 0, sizeof(mGpButtons));
}

void InputActionsMapping::SetDefaults()
{
    mControllerType = eInputControllerType_Keyboard;
    // keys in car
    for (const ActionDefaulMapping& curr: ActionsInCar)
    {
        mKeycodes[curr.mAction] = curr.mKeycode;
    }
    // keys on foot
    for (const ActionDefaulMapping& curr: ActionsOnFoot)
    {
        mKeycodes[curr.mAction] = curr.mKeycode;
    }
}

void InputActionsMapping::SetFromConfig(cxx::config_node& configNode)
{
    const char* controller_type_str = configNode.get_child("controller_type").get_value_string();
    if (!cxx::parse_enum(controller_type_str, mControllerType))
    {
        gConsole.LogMessage(eLogMessage_Warning, "Unknown controller type '%s'", controller_type_str);
    }

    // scan keycodes
    if (cxx::config_node keysNode = configNode.get_child("keys"))
    {
        ePedestrianAction action = ePedestrianAction_null;
        eKeycode keycode = eKeycode_null;

        for (cxx::config_node currNode = keysNode.first_child(); currNode; currNode = currNode.next_sibling())
        {
            const char* action_str = currNode.get_element_name();
            if (!cxx::parse_enum(action_str, action))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Unknown action %s", action_str);
                continue; 
            }
            const char* keycode_str = currNode.get_value_string();
            if (!cxx::parse_enum(keycode_str, keycode))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Unknown keycode %s", keycode_str);
                continue;
            }
            mKeycodes[action] = keycode;
        }
    }
    
    // scan gamepad buttons
    if (cxx::config_node gpNode = configNode.get_child("gamepad"))
    {
        ePedestrianAction action = ePedestrianAction_null;
        eGamepadButton gpButton = eGamepadButton_null;

        for (cxx::config_node currNode = gpNode.first_child(); currNode; currNode = currNode.next_sibling())
        {
            const char* action_str = currNode.get_element_name();
            if (!cxx::parse_enum(action_str, action))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Unknown action %s", action_str);
                continue; 
            }
            const char* gpbutton_str = currNode.get_value_string();
            if (!cxx::parse_enum(gpbutton_str, gpButton))
            {
                gConsole.LogMessage(eLogMessage_Warning, "Unknown gamepad button %s", gpbutton_str);
                continue;
            }
            mGpButtons[action] = gpButton;
        }
    }

    int bp = 0;
}

ePedestrianAction InputActionsMapping::GetAction(ePedActionsGroup group, eKeycode keycode) const
{
    if (group == ePedActionsGroup_InCar)
    {
        for (const ActionDefaulMapping& curr: ActionsInCar)
        {
            if (mKeycodes[curr.mAction] == keycode)
                return curr.mAction;
        }
        return ePedestrianAction_null;
    }

    if (group == ePedActionsGroup_OnFoot)
    {
        for (const ActionDefaulMapping& curr: ActionsOnFoot)
        {
            if (mKeycodes[curr.mAction] == keycode)
                return curr.mAction;
        }
        return ePedestrianAction_null;
    }
    return ePedestrianAction_null;
}

ePedestrianAction InputActionsMapping::GetAction(ePedActionsGroup group, eGamepadButton gpButton) const
{
    if (group == ePedActionsGroup_InCar)
    {
        for (const ActionDefaulMapping& curr: ActionsInCar)
        {
            if (mGpButtons[curr.mAction] == gpButton)
                return curr.mAction;
        }
        return ePedestrianAction_null;
    }

    if (group == ePedActionsGroup_OnFoot)
    {
        for (const ActionDefaulMapping& curr: ActionsOnFoot)
        {
            if (mGpButtons[curr.mAction] == gpButton)
                return curr.mAction;
        }
        return ePedestrianAction_null;
    }
    return ePedestrianAction_null;
}

//////////////////////////////////////////////////////////////////////////

void HumanCharacterController::UpdateFrame(Pedestrian* pedestrian, Timespan deltaTime)
{
    debug_assert(pedestrian == mCharacter);
}

void HumanCharacterController::InputEvent(KeyInputEvent& inputEvent)
{
    debug_assert(mCharacter);
    if (mInputs.mControllerType != eInputControllerType_Keyboard)
        return;

    ePedActionsGroup actionGroup = mCharacter->IsCarPassenger() ? ePedActionsGroup_InCar : ePedActionsGroup_OnFoot;
    ePedestrianAction action = mInputs.GetAction(actionGroup, inputEvent.mKeycode);
    if (action == ePedestrianAction_null)
        return;

    if (HandleInputAction(action, inputEvent.mPressed))
    {
        inputEvent.SetConsumed();
        return;
    }
}

void HumanCharacterController::InputEvent(GamepadInputEvent& inputEvent)
{
    debug_assert(mCharacter);

    static const eInputControllerType gamepadControllers[MAX_GAMEPADS] =
    {
        eInputControllerType_Gamepad1,
        eInputControllerType_Gamepad2,
        eInputControllerType_Gamepad3,
        eInputControllerType_Gamepad4
    };

    if (inputEvent.mGamepad < MAX_GAMEPADS)
    {
        eInputControllerType controllerType = gamepadControllers[inputEvent.mGamepad];
        if (controllerType != mInputs.mControllerType)
            return;

        ePedActionsGroup actionGroup = mCharacter->IsCarPassenger() ? ePedActionsGroup_InCar : ePedActionsGroup_OnFoot;
        ePedestrianAction action = mInputs.GetAction(actionGroup, inputEvent.mButton);
        if (action == ePedestrianAction_null)
            return;

        if (HandleInputAction(action, inputEvent.mPressed))
        {
            inputEvent.SetConsumed();
            return;
        }
    }
}

bool HumanCharacterController::HandleInputAction(ePedestrianAction action, bool isActivated)
{    
    switch (action)
    {
        case ePedestrianAction_SteerLeft:
        case ePedestrianAction_TurnLeft:
        case ePedestrianAction_SteerRight:
        case ePedestrianAction_TurnRight:
        case ePedestrianAction_Run:
        case ePedestrianAction_Accelerate:
        case ePedestrianAction_Reverse:
        case ePedestrianAction_WalkBackward:
        case ePedestrianAction_WalkForward:
        case ePedestrianAction_Jump:
        case ePedestrianAction_HandBrake:
        case ePedestrianAction_Shoot:
            mCharacter->mCtlActions[action] = isActivated;
        break;

        case ePedestrianAction_Horn:
            if (mCharacter->IsCarPassenger())
            {
                mCharacter->mCtlActions[ePedestrianAction_Horn] = isActivated;
                if (mCharacter->mCurrentCar->HasEmergencyLightsAnimation())
                {
                    mCharacter->mCurrentCar->EnableEmergencyLights(isActivated);
                }
            }
        break;

        case ePedestrianAction_NextWeapon:
            if (isActivated)
            {
                SwitchNextWeapon();
            }
        break;

        case ePedestrianAction_PrevWeapon:
            if (isActivated)
            {
                SwitchPrevWeapon();
            }
        break;

        case ePedestrianAction_EnterCar:
        case ePedestrianAction_LeaveCar:
        case ePedestrianAction_EnterCarAsPassenger:
            if (isActivated)
            {
                EnterOrExitCar(action == ePedestrianAction_EnterCarAsPassenger);
            }
        break;

        default:    
            debug_assert(false);
        return false;
    }

    return true;
}

void HumanCharacterController::SetCharacter(Pedestrian* character)
{
    if (mCharacter)
    {
        debug_assert(mCharacter->mController == this);
        mCharacter->mController = nullptr;
    }
    mCharacter = character;
    if (mCharacter)
    {
        mCharacter->mController = this;
    }
}

void HumanCharacterController::SwitchNextWeapon()
{
    int nextWeaponIndex = (mCharacter->mCurrentWeapon + 1) % eWeaponType_COUNT;
    for (; nextWeaponIndex != mCharacter->mCurrentWeapon; )
    {
        if (mCharacter->mWeaponsAmmo[nextWeaponIndex] != 0)
        {
            mCharacter->ChangeWeapon((eWeaponType) nextWeaponIndex);
            return;
        }
        nextWeaponIndex = (nextWeaponIndex + 1) % eWeaponType_COUNT;
    }
}

void HumanCharacterController::SwitchPrevWeapon()
{
    int nextWeaponIndex = mCharacter->mCurrentWeapon == 0 ? (eWeaponType_COUNT - 1) : (mCharacter->mCurrentWeapon - 1);
    for (; nextWeaponIndex != mCharacter->mCurrentWeapon; )
    {
        if (mCharacter->mWeaponsAmmo[nextWeaponIndex] != 0)
        {
            mCharacter->ChangeWeapon((eWeaponType) nextWeaponIndex);
            return;
        }
        nextWeaponIndex = nextWeaponIndex == 0 ? (eWeaponType_COUNT - 1) : (nextWeaponIndex - 1);
    }
}

void HumanCharacterController::EnterOrExitCar(bool alternative)
{
    if (mCharacter->IsCarPassenger())
    {
        mCharacter->LeaveCar();
        return;
    }

    PhysicsQueryResult queryResult;

    glm::vec3 pos = mCharacter->mPhysicsComponent->GetPosition();
    glm::vec2 posA { pos.x, pos.z };
    glm::vec2 posB = posA + (mCharacter->mPhysicsComponent->GetSignVector() * gGameRules.mPedestrianSpotTheCarDistance);

    gPhysics.QueryObjects(posA, posB, queryResult);

    // process all cars
    for (int icar = 0; icar < queryResult.mCarsCount; ++icar)
    {
        Vehicle* currCar = queryResult.mCarsList[icar]->mReferenceCar;

        mCharacter->TakeSeatInCar(currCar, alternative ? eCarSeat_Passenger : eCarSeat_Driver);
        return;
    }
}