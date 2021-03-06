// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
 * @file RTPSWriter.cpp
 *
 */

#include <fastrtps/rtps/writer/RTPSWriter.h>
#include <fastrtps/rtps/history/WriterHistory.h>
#include <fastrtps/rtps/messages/RTPSMessageCreator.h>
#include <fastrtps/log/Log.h>
#include "../participant/RTPSParticipantImpl.h"

#include <mutex>

using namespace eprosima::fastrtps::rtps;


RTPSWriter::RTPSWriter(RTPSParticipantImpl* impl, GUID_t& guid, WriterAttributes& att, WriterHistory* hist, WriterListener* listen):
    Endpoint(impl,guid,att.endpoint),
    m_pushMode(true),
    m_cdrmessages(impl->getMaxMessageSize() > att.throughputController.bytesPerPeriod ?
            att.throughputController.bytesPerPeriod > impl->getRTPSParticipantAttributes().throughputController.bytesPerPeriod ?
            impl->getRTPSParticipantAttributes().throughputController.bytesPerPeriod :
            att.throughputController.bytesPerPeriod :
            impl->getMaxMessageSize() > impl->getRTPSParticipantAttributes().throughputController.bytesPerPeriod ?
            impl->getRTPSParticipantAttributes().throughputController.bytesPerPeriod :
            impl->getMaxMessageSize()),
    m_livelinessAsserted(false),
    mp_history(hist),
    mp_listener(listen),
    is_async_(att.mode == SYNCHRONOUS_WRITER ? false : true)
{
    mp_history->mp_writer = this;
    mp_history->mp_mutex = mp_mutex;
    this->init_header();
    logInfo(RTPS_WRITER,"RTPSWriter created");
}

void RTPSWriter::init_header()
{
    CDRMessage::initCDRMsg(&m_cdrmessages.m_rtpsmsg_header);
    RTPSMessageCreator::addHeader(&m_cdrmessages.m_rtpsmsg_header,m_guid.guidPrefix);
}


RTPSWriter::~RTPSWriter()
{
    logInfo(RTPS_WRITER,"RTPSWriter destructor");

    // Deletion of the events has to be made in child destructor.

    mp_history->mp_writer = nullptr;
    mp_history->mp_mutex = nullptr;
}

CacheChange_t* RTPSWriter::new_change(const std::function<uint32_t()>& dataCdrSerializedSize,
        ChangeKind_t changeKind, InstanceHandle_t handle)
{
    logInfo(RTPS_WRITER,"Creating new change");
    CacheChange_t* ch = nullptr;

    if(!mp_history->reserve_Cache(&ch, dataCdrSerializedSize))
    {
        logWarning(RTPS_WRITER,"Problem reserving Cache from the History");
        return nullptr;
    }

    ch->kind = changeKind;
    if(m_att.topicKind == WITH_KEY && !handle.isDefined())
    {
        logWarning(RTPS_WRITER,"Changes in KEYED Writers need a valid instanceHandle");
    }
    ch->instanceHandle = handle;
    ch->writerGUID = m_guid;
    return ch;
}

SequenceNumber_t RTPSWriter::get_seq_num_min()
{
    CacheChange_t* change;
    if(mp_history->get_min_change(&change) && change!= nullptr)
        return change->sequenceNumber;
    else
        return c_SequenceNumber_Unknown;
}

SequenceNumber_t RTPSWriter::get_seq_num_max()
{
    CacheChange_t* change;
    if(mp_history->get_max_change(&change) && change!=nullptr)
        return change->sequenceNumber;
    else
        return c_SequenceNumber_Unknown;
}

uint32_t RTPSWriter::getTypeMaxSerialized()
{
    return mp_history->getTypeMaxSerialized();
}


bool RTPSWriter::remove_older_changes(unsigned int max)
{
    logInfo(RTPS_WRITER, "Starting process clean_history for writer " << getGuid());
    std::lock_guard<std::recursive_mutex> guard(*mp_mutex);
    bool limit = (max != 0);

    bool remove_ret = mp_history->remove_min_change();
    bool at_least_one = remove_ret;
    unsigned int count = 1;

    while(remove_ret && (!limit || count < max))
    {
        remove_ret = mp_history->remove_min_change();
        ++count;
    }

    return at_least_one;
}
