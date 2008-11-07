<?xml version='1.0'?>
<!-- Order of specification will determine the sequence of installation. all modules are loaded prior instantiation of plugins -->
<xp:Profile xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:soapenc="http://schemas.xmlsoap.org/soap/encoding/" xmlns:xp="http://xdaq.web.cern.ch/xdaq/xsd/2005/XMLProfile-10">
	<!-- Compulsory  Plugins -->
	<xp:Application class="executive::Application" id="0" group="profile" service="executive" network="local">
		<properties xmlns="urn:xdaq-application:Executive" xsi:type="soapenc:Struct">
			<logUrl xsi:type="xsd:string">console</logUrl>
                	<logLevel xsi:type="xsd:string">INFO</logLevel>
                </properties>
	</xp:Application>
	<xp:Module>${XDAQ_ROOT}/lib/libexecutive.so</xp:Module>
	
	<xp:Application class="pt::http::PeerTransportHTTP" id="1" group="profile" network="local">
		 <properties xmlns="urn:xdaq-application:pt::http::PeerTransportHTTP" xsi:type="soapenc:Struct">
		 	<documentRoot xsi:type="xsd:string">${XDAQ_DOCUMENT_ROOT}</documentRoot>
                        <aliasName xsi:type="xsd:string">tmp</aliasName>
                        <aliasPath xsi:type="xsd:string">/tmp</aliasPath>
                </properties>
	</xp:Application>
	<xp:Module>${XDAQ_ROOT}/lib/libpthttp.so</xp:Module>

	<xp:Application class="pt::fifo::PeerTransportFifo" id="8" group="profile" network="local"/>
	<xp:Module>${XDAQ_ROOT}/lib/libptfifo.so</xp:Module>
	

	<!-- DABC specific libraries -->
	<xp:Module>${DIMDIR}/linux/libdim.so</xp:Module>
	<xp:Module>${DABCSYS}/lib/libDabcBase.so</xp:Module>
	<xp:Module>${DABCSYS}/lib/libDabcXDAQControl.so</xp:Module>   

</xp:Profile>
