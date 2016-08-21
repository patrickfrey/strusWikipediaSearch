{% extends "search_base_html.tpl" %}

{% block infoblock %}
{% if message != None and len(message) > 0 %}
<div id="searcherror">
<p>Error: {{message}}</p>
</div>
{% end %}
{% end %}
{% block resultblock %}
<div id="searchresult">
<ul>
{% for result in results %}
<li>
<div id="rank">
<div id="rank_title"> {{ result }} </div><br/>
</div>
</li>
{% end %}
</ul>
</div>
{% end %}

