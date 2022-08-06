#include "StdInc.h"

#include "Garage.h"

void CGarage::InjectHooks() {
    RH_ScopedClass(CGarage);
    RH_ScopedCategoryGlobal();

    RH_ScopedInstall(BuildRotatedDoorMatrix, 0x4479F0);
    // RH_ScopedInstall(TidyUpGarageClose, 0x449D10);
    // RH_ScopedInstall(TidyUpGarage, 0x449C50);
    RH_ScopedInstall(StoreAndRemoveCarsForThisHideOut, 0x449900);
    // RH_ScopedInstall(EntityHasASphereWayOutsideGarage, 0x449050);
    // RH_ScopedInstall(RemoveCarsBlockingDoorNotInside, 0x449690);
    // RH_ScopedInstall(IsEntityTouching3D, 0x448EE0);
    // RH_ScopedInstall(IsEntityEntirelyOutside, 0x448D30);
    // RH_ScopedInstall(IsEntityEntirelyInside3D, 0x448BE0);
    // RH_ScopedInstall(IsStaticPlayerCarEntirelyInside, 0x44A830);
    // RH_ScopedInstall(IsPointInsideGarage, 0x448740);
    // RH_ScopedInstall(PlayerArrestedOrDied, 0x4486C0);
    RH_ScopedInstall(OpenThisGarage, 0x447D50);
    RH_ScopedInstall(CloseThisGarage, 0x447D70);
    // RH_ScopedInstall(InitDoorsAtStart, 0x447600);
    // RH_ScopedInstall(IsPointInsideGarage, 0x4487D0);
    // RH_ScopedInstall(Update, 0x44AA50);
}

// 0x4479F0
void CGarage::BuildRotatedDoorMatrix(CEntity* entity, float fDoorPosition) {
    const auto fAngle = fDoorPosition * -HALF_PI;
    const auto fSin = std::sin(fAngle);
    const auto fCos = std::cos(fAngle);
    CMatrix& matrix = entity->GetMatrix();

    const auto& vecForward = matrix.GetForward();
    matrix.GetUp() = CVector(-fSin * vecForward.y, fSin * vecForward.x, fCos);
    matrix.GetRight() = CrossProduct(vecForward, matrix.GetUp());
}

// 0x449D10
void CGarage::TidyUpGarageClose() {
    plugin::CallMethod<0x449D10, CGarage*>(this);
}

// 0x449C50
void CGarage::TidyUpGarage() {
    plugin::CallMethod<0x449C50, CGarage*>(this);
}

// 0x449900
void CGarage::StoreAndRemoveCarsForThisHideOut(CStoredCar* storedCars, int32 maxSlot) {
    maxSlot = std::min(maxSlot, NUM_GARAGE_STORED_CARS);

    for (auto i = 0; i < NUM_GARAGE_STORED_CARS; i++) {
        storedCars[i].Clear();
    }

    auto storedCarIdx{0u};
    for (auto i = 0; i < GetVehiclePool()->GetSize(); i++) {
        auto vehicle = GetVehiclePool()->GetAt(i);
        if (!vehicle)
            continue;

        if (IsPointInsideGarage(vehicle->GetPosition()) && !vehicle->IsCreatedBy(MISSION_VEHICLE)) {
            if (storedCarIdx < maxSlot && !EntityHasASphereWayOutsideGarage(vehicle, 1.0f)) {
                storedCars[storedCarIdx++].StoreCar(vehicle);
            }

            FindPlayerInfo().CancelPlayerEnteringCars(vehicle);
            CWorld::Remove(vehicle);
            delete vehicle;
        }
    }

    // Clear slots with no vehicles in it
    for (auto i = storedCarIdx; i < NUM_GARAGE_STORED_CARS; i++) {
        storedCars[i].Clear();
    }
}

// 0x449050
bool CGarage::EntityHasASphereWayOutsideGarage(CEntity* entity, float fRadius) {
    return plugin::CallMethodAndReturn<bool, 0x449050, CGarage*, CEntity*, float>(this, entity, fRadius);
}

// 0x449690
void CGarage::RemoveCarsBlockingDoorNotInside() {
    plugin::CallMethod<0x449690, CGarage*>(this);
}

// 0x448EE0
bool CGarage::IsEntityTouching3D(CEntity* entity) {
    return plugin::CallMethodAndReturn<bool, 0x448EE0, CGarage*, CEntity*>(this, entity);

    const auto cm = entity->GetModelInfo()->GetColModel();
    const auto& radius = cm->GetBoundRadius();
    const auto& pos = entity->GetPosition();

    if (m_fLeftCoord - radius > pos.x || radius + m_fRightCoord < pos.x)
        return false;
    if (m_fFrontCoord - radius > pos.y || radius + m_fBackCoord < pos.y)
        return false;
    if (m_vPosn.z - radius > pos.z || radius + m_fTopZ < pos.z)
        return false;

    for (auto& sphere : cm->m_pColData->GetSpheres()) {
        CVector point = MultiplyMatrixWithVector(entity->GetMatrix(), sphere.m_vecCenter);
        if (IsPointInsideGarage(point, sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x44A830
bool CGarage::IsStaticPlayerCarEntirelyInside() {
    return plugin::CallMethodAndReturn<bool, 0x44A830, CGarage*>(this);

    auto vehicle = FindPlayerVehicle();
    if (!vehicle)
        return false;
    if (!vehicle->IsAutomobile() && !vehicle->IsBike())
        return false;

    auto player = FindPlayerPed();
    if (player->GetTaskManager().FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR))
        return false;

    const auto& pos = vehicle->GetPosition();
    if (pos.x < m_fLeftCoord || pos.x > m_fRightCoord)
        return false;
    if (pos.y < m_fFrontCoord || pos.y > m_fBackCoord)
        return false;

    auto x = vehicle->m_vecMoveSpeed.x;
    if (x < 0.0f)
        x = -x;
    if (x > 0.01f)
        return false;

    auto y = vehicle->m_vecMoveSpeed.y;
    if (y < 0.0f)
        y = -y;
    if (y > 0.01f)
        return false;

    auto z = vehicle->m_vecMoveSpeed.z;
    if (z < 0.0f)
        z = -z;
    if (z > 0.01f)
        return false;

    if (CVector{ x, y, z }.SquaredMagnitude() > sq(0.01f))
        return false;

    return IsEntityEntirelyInside3D(vehicle, 0.0f);
}

// Doesn't check the height against the IsEntityEntirelyInside3D.
// Also, can't use inverted IsEntityEntirelyInside3D due m_pColData check.
// 0x448D30
bool CGarage::IsEntityEntirelyOutside(CEntity* entity, float radius) {
    return plugin::CallMethodAndReturn<bool, 0x448D30, CGarage*, CEntity*, float>(this, entity, radius);

    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius < pos.x || radius + m_fRightCoord > pos.x)
        return false;
    if (m_fFrontCoord - radius < pos.y || radius + m_fBackCoord > pos.y)
        return false;

    auto cd = entity->GetColModel()->m_pColData;
    if (!cd) return true;

    for (const auto& sphere : cd->GetSpheres()) {
        CVector out = MultiplyMatrixWithVector(entity->GetMatrix(), sphere.m_vecCenter);
        if (!IsPointInsideGarage(out, radius + sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x448BE0
bool CGarage::IsEntityEntirelyInside3D(CEntity* entity, float radius) {
    return plugin::CallMethodAndReturn<bool, 0x448BE0, CGarage*, CEntity*, float>(this, entity, radius);

    const auto& pos = entity->GetPosition();
    if (m_fLeftCoord - radius > pos.x || radius + m_fRightCoord < pos.x)
        return false;
    if (m_fFrontCoord - radius > pos.y || radius + m_fBackCoord < pos.y)
        return false;
    if (m_vPosn.z - radius > pos.z || radius + m_fTopZ < pos.z)
        return false;

    auto cd = entity->GetColModel()->m_pColData;
    if (!cd) return true;

    for (const auto& sphere : cd->GetSpheres()) {
        CVector out = MultiplyMatrixWithVector(entity->GetMatrix(), sphere.m_vecCenter);
        if (IsPointInsideGarage(out, radius - sphere.m_fRadius)) {
            return true;
        }
    }
    return false;
}

// 0x4486C0
eGarageDoorState CGarage::PlayerArrestedOrDied() {
    return plugin::CallMethodAndReturn<eGarageDoorState, 0x4486C0, CGarage*>(this);
}

// 0x447D50
void CGarage::OpenThisGarage() {
    if (m_nDoorState == GARAGE_DOOR_CLOSED ||
        m_nDoorState == GARAGE_DOOR_CLOSING ||
        m_nDoorState == GARAGE_DOOR_CLOSED_DROPPED_CAR
    ) {
        m_nDoorState = GARAGE_DOOR_OPENING;
    }
}

// 0x447D70
void CGarage::CloseThisGarage() {
    if (m_nDoorState == GARAGE_DOOR_OPEN || m_nDoorState == GARAGE_DOOR_OPENING) {
        m_nDoorState = GARAGE_DOOR_CLOSING;
    }
}

// 0x447600
void CGarage::InitDoorsAtStart() {
    plugin::CallMethod<0x447600, CGarage*>(this);
}

// 0x448740
bool CGarage::IsPointInsideGarage(CVector point) {
    return plugin::CallMethodAndReturn<bool, 0x448740, CGarage*, CVector>(this, point);

    if (point.z < m_vPosn.z || point.z > m_fTopZ)
        return false;

    const auto x0 = point.x - m_vPosn.x;
    const auto y0 = point.y - m_vPosn.y;

    auto sqMagA = x0 * m_vDirectionA.x + y0 * m_vDirectionA.y;
    if (sqMagA < 0.0f || sqMagA > m_fWidth)
        return false;

    auto sqMagB = x0 * m_vDirectionB.x + y0 * m_vDirectionB.y;
    if (sqMagB < 0.0f || sqMagB > m_fHeight)
        return false;

    return 1;
}

// See CZoneDef::IsPointWithin
// 0x4487D0
bool CGarage::IsPointInsideGarage(CVector point, float radius) {
    return plugin::CallMethodAndReturn<bool, 0x4487D0, CGarage*, CVector, float>(this, point, radius);

    if (m_vPosn.z - radius > point.z || radius + m_fTopZ < point.z)
        return false;

    const auto x0 = point.x - m_vPosn.x;
    const auto y0 = point.y - m_vPosn.y;

    auto sqMagA = x0 * m_vDirectionA.x + y0 * m_vDirectionA.y;
    if (sqMagA < -radius || sqMagA > radius + m_fWidth)
        return false;

    auto sqMagB = x0 * m_vDirectionB.x + y0 * m_vDirectionB.y;
    if (sqMagB < -radius || sqMagB > radius + m_fHeight)
        return false;

    return true;
}

// 0x44AA50
void CGarage::Update(int32 garageId) {
    plugin::CallMethod<0x44AA50, CGarage*>(this, garageId);
}

bool CGarage::IsHideOut() const {
    switch (m_nType) {
    case eGarageType::SAFEHOUSE_GANTON:
    case eGarageType::SAFEHOUSE_SANTAMARIA:
    case eGarageType::SAGEHOUSE_ROCKSHORE:
    case eGarageType::SAFEHOUSE_FORTCARSON:
    case eGarageType::SAFEHOUSE_VERDANTMEADOWS:
    case eGarageType::SAFEHOUSE_DILLIMORE:
    case eGarageType::SAFEHOUSE_PRICKLEPINE:
    case eGarageType::SAFEHOUSE_WHITEWOOD:
    case eGarageType::SAFEHOUSE_PALOMINOCREEK:
    case eGarageType::SAFEHOUSE_REDSANDSWEST:
    case eGarageType::SAFEHOUSE_ELCORONA:
    case eGarageType::SAFEHOUSE_MULHOLLAND:
    case eGarageType::SAFEHOUSE_CALTONHEIGHTS:
    case eGarageType::SAFEHOUSE_PARADISO:
    case eGarageType::SAFEHOUSE_DOHERTY:
    case eGarageType::SAFEHOUSE_HASHBURY:
    case eGarageType::HANGAR_ABANDONED_AIRPORT:
        return true;
    default:
        return false;
    }
}

// 0x44A9C0
bool CGarage::IsGarageEmpty() {
    return plugin::CallMethodAndReturn<bool, 0x44A9C0, CGarage*>(this);

    CVector cornerA = { m_fLeftCoord, m_fFrontCoord, m_vPosn.z };
    CVector cornerB = { m_fRightCoord, m_fBackCoord, m_fTopZ   };

    int16 outCount[2];
    CEntity* outEntities[16];
    CWorld::FindObjectsIntersectingCube(&cornerA, &cornerB, outCount, std::size(outEntities), outEntities, false, true, true, false, false);
    if (outCount[0] <= 0)
        return true;

    int16 entityIndex = 0;

    while (!IsEntityTouching3D(outEntities[entityIndex])) {
        if (++entityIndex >= outCount[0])
            return true;
    }
    return false;
}

// 0x449100
bool CGarage::IsAnyOtherCarTouchingGarage(CVehicle* vehicle) {
    return plugin::CallMethodAndReturn<bool, 0x449100, CGarage*, CVehicle*>(this, vehicle);
}

// 0x4493E0
bool CGarage::IsAnyOtherPedTouchingGarage(CPed* ped) {
    return plugin::CallMethodAndReturn<bool, 0x4493E0, CGarage*, CPed*>(this, ped);
}

// WIP?
// 0x449E90
void CGarage::CenterCarInGarage(CVehicle* vehicle) {
    if (IsAnyOtherCarTouchingGarage(FindPlayerVehicle()) || IsAnyOtherPedTouchingGarage(FindPlayerPed()))
        return;

    const auto& pos = vehicle->GetPosition();
    const auto halfX = (m_fRightCoord + m_fLeftCoord) / 2.0f;
    const auto halfY = (m_fBackCoord + m_fFrontCoord) / 2.0f;
    CVector p1{
        halfX - pos.x,
        halfY - pos.y,
        pos.z - pos.z
    };

    auto dist = p1.Magnitude();
    if (dist >= 0.4f) {
        auto x = halfX - pos.x * 0.4f / dist + pos.x;
        auto y = 0.4f / dist * halfY - pos.y + pos.y;
    } else {
        auto x = halfX;
        auto y = halfY;
    }

    if (!IsEntityEntirelyInside3D(vehicle, 0.3f)) {
        vehicle->SetPosn(pos);
    }
}

// 0x5D3020
void CSaveGarage::CopyGarageIntoSaveGarage(Const CGarage& g) {
    m_nType         = g.m_nType;
    m_nDoorState    = g.m_nDoorState;
    m_nFlags        = g.m_nFlags;
    m_vPosn         = g.m_vPosn;
    m_vDirectionA   = g.m_vDirectionA;
    m_vDirectionB   = g.m_vDirectionB;
    m_fTopZ         = g.m_fTopZ;
    m_fWidth        = g.m_fWidth;
    m_fHeight       = g.m_fHeight;
    m_fLeftCoord    = g.m_fLeftCoord;
    m_fRightCoord   = g.m_fRightCoord;
    m_fFrontCoord   = g.m_fFrontCoord;
    m_fBackCoord    = g.m_fBackCoord;
    m_fDoorPosition = g.m_fDoorPosition;
    m_nTimeToOpen   = g.m_nTimeToOpen;
    m_nOriginalType = g.m_nOriginalType;
    strcpy_s(m_anName, g.m_anName);
}

// 0x5D30C0
void CSaveGarage::CopyGarageOutOfSaveGarage(CGarage& g) const {
    g.m_nType         = m_nType;
    g.m_nDoorState    = m_nDoorState;
    g.m_nFlags        = m_nFlags;
    g.m_vPosn         = m_vPosn;
    g.m_vDirectionA   = m_vDirectionA;
    g.m_vDirectionB   = m_vDirectionB;
    g.m_fTopZ         = m_fTopZ;
    g.m_fWidth        = m_fWidth;
    g.m_fHeight       = m_fHeight;
    g.m_fLeftCoord    = m_fLeftCoord;
    g.m_fRightCoord   = m_fRightCoord;
    g.m_fFrontCoord   = m_fFrontCoord;
    g.m_fBackCoord    = m_fBackCoord;
    g.m_fDoorPosition = m_fDoorPosition;
    g.m_nTimeToOpen   = m_nTimeToOpen;
    g.m_nOriginalType = m_nOriginalType;
    g.m_pTargetCar    = nullptr;
    strcpy_s(g.m_anName, m_anName);
}
