{% extends "search_base_html.tpl" %}

{% block resultblock %}
<div id="navigation">
<div id="logo">
 <a target="_blank" href="http://project-strus.net">
  <img width="100%" src="static/strus_logo.jpg" alt="strus logo"/>
<!-- Copyright: <a href='http://www.123rf.com/profile_guarding123'>guarding123 / 123RF Stock Photo</a>
-->
 </a>
</div>
<div id="navbar">
<div id="navbox">
<button id="back" onclick="window.history.back()">Back</button>
</div>
</div>
</div>
<div id="searcherror">
<p>Error: {{message}}</p>
</div>
{% end %}


