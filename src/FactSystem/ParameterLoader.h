/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef PARAMETERLOADER_H
#define PARAMETERLOADER_H

#include <QObject>
#include <QMap>
#include <QXmlStreamReader>
#include <QLoggingCategory>
#include <QMutex>

#include "FactSystem.h"
#include "MAVLinkProtocol.h"
#include "AutoPilotPlugin.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

Q_DECLARE_LOGGING_CATEGORY(ParameterLoaderLog)
Q_DECLARE_LOGGING_CATEGORY(ParameterLoaderVerboseLog)

/// Connects to Parameter Manager to load/update Facts
class ParameterLoader : public QObject
{
    Q_OBJECT
    
public:
    /// @param uas Uas which this set of facts is associated with
    ParameterLoader(AutoPilotPlugin* autopilot, Vehicle* vehicle, QObject* parent = NULL);
    
    ~ParameterLoader();

    /// @return Location of parameter cache file
    static QString parameterCacheFile(void);
    
    /// Returns true if the full set of facts are ready
    bool parametersAreReady(void) { return _parametersReady; }

    /// Re-request the full set of parameters from the autopilot
    void refreshAllParameters(uint8_t componentID = MAV_COMP_ID_ALL);

    /// Request a refresh on the specific parameter
    void refreshParameter(int componentId, const QString& name);
    
    /// Request a refresh on all parameters that begin with the specified prefix
    void refreshParametersPrefix(int componentId, const QString& namePrefix);
    
    /// Returns true if the specifed parameter exists
    bool parameterExists(int			componentId,    ///< fact component, -1=default component
						 const QString& name);          ///< fact name
	
	/// Returns all parameter names
	QStringList parameterNames(int componentId);
    
    /// Returns the specified Fact.
    /// WARNING: Will assert if parameter does not exists. If that possibily exists, check for existence first with
    /// parameterExists.
    Fact* getFact(int               componentId,    ///< fact component, -1=default component
                  const QString&    name);          ///< fact name
    
    const QMap<int, QMap<QString, QStringList> >& getGroupMap(void);
    
    /// Returns error messages from loading
    QString readParametersFromStream(QTextStream& stream);
    
    void writeParametersToStream(QTextStream &stream);
    
signals:
    /// Signalled when the full set of facts are ready
    void parametersReady(bool missingParameters);

    /// Signalled to update progress of full parameter list request
    void parameterListProgress(float value);
    
    /// Signalled to ourselves in order to get call on our own thread
    void restartWaitingParamTimer(void);
    
protected:
    AutoPilotPlugin*    _autopilot;
    Vehicle*            _vehicle;
    MAVLinkProtocol*    _mavlink;
    
    void _parameterUpdate(int uasId, int componentId, QString parameterName, int parameterCount, int parameterId, int mavType, QVariant value);
    void _valueUpdated(const QVariant& value);
    void _restartWaitingParamTimer(void);
    void _waitingParamTimeout(void);
    void _tryCacheLookup(void);
    void _initialRequestTimeout(void);
    
private slots:
    void _timeoutRefreshAll();

private:
    static QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool failOk = false);
    int _actualComponentId(int componentId);
    void _determineDefaultComponentId(void);
    void _setupGroupMap(void);
    void _readParameterRaw(int componentId, const QString& paramName, int paramIndex);
    void _writeParameterRaw(int componentId, const QString& paramName, const QVariant& value);
    void _writeLocalParamCache();
    void _tryCacheHashLoad(int uasId, QVariant hash_value);

    MAV_PARAM_TYPE _factTypeToMavType(FactMetaData::ValueType_t factType);
    FactMetaData::ValueType_t _mavTypeToFactType(MAV_PARAM_TYPE mavType);
    void _saveToEEPROM(void);
    void _checkInitialLoadComplete(void);
    
    /// First mapping is by component id
    /// Second mapping is parameter name, to Fact* in QVariant
    QMap<int, QVariantMap>            _mapParameterName2Variant;
    QMap<int, QMap<int, QString> >    _mapParameterId2Name;
    
    /// First mapping is by component id
    /// Second mapping is group name, to Fact
    QMap<int, QMap<QString, QStringList> > _mapGroup2ParameterName;
    
    bool _parametersReady;      ///< true: full set of parameters correctly loaded
    bool _initialLoadComplete;  ///< true: Initial load of all parameters complete, whether succesful or not
    bool _saveRequired;         ///< true: _saveToEEPROM should be called
    int _defaultComponentId;
    QString _defaultComponentIdParam;
    
    static const int _maxInitialLoadRetry = 10;                  ///< Maximum a retries on initial index based load
    
    QMap<int, int>                  _paramCountMap;             ///< Key: Component id, Value: count of parameters in this component
    QMap<int, QMap<int, int> >      _waitingReadParamIndexMap;  ///< Key: Component id, Value: Map { Key: parameter index still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingReadParamNameMap;   ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QMap<QString, int> >  _waitingWriteParamNameMap;  ///< Key: Component id, Value: Map { Key: parameter name still waiting for, Value: retry count }
    QMap<int, QList<int> >          _failedReadParamIndexMap;   ///< Key: Component id, Value: failed parameter index
    
    int _totalParamCount;   ///< Number of parameters across all components
    
    QTimer _initialRequestTimeoutTimer;
    QTimer _waitingParamTimeoutTimer;
    QTimer _cacheTimeoutTimer;
    
    QMutex _dataMutex;
    
    static Fact _defaultFact;   ///< Used to return default fact, when parameter not found
};

#endif
